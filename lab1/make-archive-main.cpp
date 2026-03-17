#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include <ProcessManager.hpp>

namespace fs = std::filesystem;

struct ProgramArgs
{
	int maxProcesses = -1;
	bool isValid = false;

	std::string archiveName{};
	std::vector<std::string> inputItems{};
};

void PrintUsage()
{
	std::cerr << "Usage:\n"
			  << "  make-archive -S ARCHIVE-NAME [INPUT-FILES]\n"
			  << "  make-archive -P NUM-PROCESSES ARCHIVE-NAME [INPUT-FILES]\n";
}

ProgramArgs ParseArgs(const int argc, char* argv[])
{
	ProgramArgs args;
	if (argc < 4)
	{
		return args;
	}

	const std::string modeFlag = argv[1];
	int argIndex = 2;

	if (modeFlag == "-P")
	{
		try
		{
			args.maxProcesses = std::stoi(argv[argIndex++]);
		}
		catch (...)
		{
			return args;
		}
	}
	else if (modeFlag != "-S")
	{
		return args;
	}

	if (argIndex >= argc)
	{
		return args;
	}
	args.archiveName = argv[argIndex++];

	for (; argIndex < argc; ++argIndex)
	{
		args.inputItems.emplace_back(argv[argIndex]);
	}

	args.isValid = true;

	return args;
}

std::vector<std::string> CollectFiles(const std::vector<std::string>& inputs)
{
	std::vector<std::string> files;
	for (const auto& item : inputs)
	{
		try
		{
			if (fs::is_directory(item))
			{
				auto options = fs::directory_options::skip_permission_denied;
				for (const auto& entry : fs::recursive_directory_iterator(item, options))
				{
					if (entry.is_regular_file())
					{
						files.push_back(entry.path().string());
					}
				}
			}
			else if (fs::is_regular_file(item))
			{
				files.push_back(item);
			}
		}
		catch (const fs::filesystem_error& e)
		{
			std::cerr << "Warning: Could not process " << item << ": " << e.what() << "\n";
		}
	}

	return files;
}

std::string CreateTempDir()
{
	char tempDirPath[] = "/tmp/make_archive_XXXXXX";
	if (mkdtemp(tempDirPath) == nullptr)
	{
		return "";
	}

	return { tempDirPath };
}

void CompressFiles(ProcessManager& pm, const std::vector<std::string>& files, const fs::path& tempDir)
{
	for (const auto& inputFile : files)
	{
		fs::path targetPath = tempDir;
		for (const auto& part : fs::path(inputFile))
		{
			std::string s = part.string();
			if (s == ".." || s == "." || s == "/" || s == "\\")
			{
				continue;
			}
			targetPath /= part;
		}
		targetPath += ".gz";

		fs::create_directories(targetPath.parent_path());
		pm.RunAsync("gzip", { "-c", inputFile }, targetPath.string());
	}
	pm.WaitAll();
}

int main(const int argc, char* argv[])
{
	const auto startTotal = std::chrono::steady_clock::now();

	auto [maxProcesses, isValid, archiveName, inputItems] = ParseArgs(argc, argv);
	if (!isValid)
	{
		PrintUsage();
		return 1;
	}

	const auto filesToProcess = CollectFiles(inputItems);
	if (filesToProcess.empty())
	{
		std::cerr << "No valid files to process.\n";
		return 0;
	}

	const std::string tempDirPath = CreateTempDir();
	if (tempDirPath.empty())
	{
		std::cerr << "Failed to create temp directory\n";
		return 1;
	}
	const fs::path tempDir(tempDirPath);

	ProcessManager processManager(maxProcesses);
	CompressFiles(processManager, filesToProcess, tempDir);

	const auto startTar = std::chrono::steady_clock::now();
	processManager.RunSync("tar", { "-cf", archiveName, "-C", tempDir.string(), "." });
	const auto endTar = std::chrono::steady_clock::now();

	fs::remove_all(tempDir);

	const auto endTotal = std::chrono::steady_clock::now();
	const std::chrono::duration<double> totalTime = endTotal - startTotal;
	const std::chrono::duration<double> tarTime = endTar - startTar;

	std::cout << "Total time: " << totalTime.count() << "s\n";
	std::cout << "Tar time: " << tarTime.count() << "s\n";

	return 0;
}