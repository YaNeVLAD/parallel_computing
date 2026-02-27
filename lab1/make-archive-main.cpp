#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <ProcessManager.hpp>

namespace fs = std::filesystem;

int main(const int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "Usage:\n"
				  << "  make-archive -S ARCHIVE-NAME [INPUT-FILES]\n"
				  << "  make-archive -P NUM-PROCESSES ARCHIVE-NAME [INPUT-FILES]\n";
		return 1;
	}

	auto startTotal = std::chrono::steady_clock::now();

	int maxProcesses = 1;
	std::string archiveName;
	std::vector<std::string> rawInputItems;

	std::string modeFlag = argv[1];
	int argIndex = 2;

	if (modeFlag == "-P")
	{
		try
		{
			maxProcesses = std::stoi(argv[argIndex++]);
		}
		catch (...)
		{
			std::cerr << "Invalid NUM-PROCESSES argument\n";
			return 1;
		}
	}
	else if (modeFlag != "-S")
	{
		std::cerr << "Unknown mode: " << modeFlag << "\n";
		return 1;
	}

	archiveName = argv[argIndex++];
	for (; argIndex < argc; ++argIndex)
	{
		rawInputItems.emplace_back(argv[argIndex]);
	}

	std::vector<std::string> filesToProcess;
	for (const auto& inputItem : rawInputItems)
	{
		try
		{
			if (fs::is_directory(inputItem))
			{
				auto options = fs::directory_options::skip_permission_denied;
				for (const auto& entry : fs::recursive_directory_iterator(inputItem, options))
				{
					if (entry.is_regular_file())
					{
						filesToProcess.push_back(entry.path().string());
					}
				}
			}
			else if (fs::is_regular_file(inputItem))
			{
				filesToProcess.push_back(inputItem);
			}
			else
			{
				std::cerr << "Warning: Skipping invalid or inaccessible input: " << inputItem << "\n";
			}
		}
		catch (const fs::filesystem_error& e)
		{
			std::cerr << "Warning: Could not process " << inputItem << ": " << e.what() << "\n";
		}
	}

	if (filesToProcess.empty())
	{
		std::cerr << "No valid files to process. Exiting.\n";
		return 0;
	}

	char tempDirPath[] = "/tmp/make_archive_XXXXXX";
	if (mkdtemp(tempDirPath) == nullptr)
	{
		std::cerr << "Failed to create temporary directory\n";
		return 1;
	}
	const fs::path tempDir(tempDirPath);

	ProcessManager processManager(maxProcesses);

	for (const auto& inputFile : filesToProcess)
	{
		fs::path inputPath(inputFile);
		fs::path targetPath = tempDir;

		for (const auto& part : inputPath)
		{
			std::string partStr = part.string();
			if (partStr == ".." || partStr == "." || partStr == "/" || partStr == "\\")
			{
				continue;
			}
			targetPath /= part;
		}
		targetPath = targetPath.string() + ".gz";

		fs::create_directories(targetPath.parent_path());
		processManager.RunAsync("gzip", { "-c", inputFile }, targetPath.string());
	}

	processManager.WaitAll();

	const auto startTar = std::chrono::steady_clock::now();
	processManager.RunSync("tar", { "-cf", archiveName, "-C", tempDir.string(), "." });
	const auto endTar = std::chrono::steady_clock::now();

	fs::remove_all(tempDir);

	const auto endTotal = std::chrono::steady_clock::now();

	const std::chrono::duration<double> totalTime = endTotal - startTotal;
	const std::chrono::duration<double> tarTime = endTar - startTar;

	std::cout << "Total time: " << totalTime.count() << " seconds\n";
	std::cout << "Sequential part (tar) time: " << tarTime.count() << " seconds\n";

	return 0;
}