#include <generator>
#include <iostream>
#include <string>
#include <vector>

struct Book
{
	std::string title;
	std::string author;
	std::vector<std::string> chapters;
};

struct BookChapter
{
	std::string bookTitle;
	std::string bookAuthor;
	std::string chapterTitle;
};

std::ostream& operator<<(std::ostream& os, BookChapter const& chapter)
{
	os << chapter.bookTitle << " by " << chapter.bookAuthor << ": " << chapter.chapterTitle;

	return os;
}

std::generator<BookChapter> ListChapters(
	std::string const& title,
	std::string const& author,
	std::vector<std::string> const& chapters)
{
	for (const auto& chapter : chapters)
	{
		co_yield BookChapter{
			.bookTitle = title,
			.bookAuthor = author,
			.chapterTitle = chapter,
		};
	}
}

std::generator<BookChapter> ListBookChapters(std::vector<Book> const& books)
{
	for (auto const& [title, author, chapters] : books)
	{
		for (auto const& chapter : ListChapters(title, author, chapters))
		{
			co_yield chapter;
		}
	}
}

int main()
{
	const std::vector<Book> books = {
		{ "The Great Gatsby", "F. Scott Fitzgerald", { "Chapter 1", "Chapter 2" } },
		{ "1984", "George Orwell", { "Chapter 1", "Chapter 2", "Chapter 3" } },
		{ "To Kill a Mockingbird", "Harper Lee", { "Chapter 1" } }
	};

	for (auto const& chapter : ListBookChapters(books))
	{
		std::cout << chapter << std::endl;
	}
}