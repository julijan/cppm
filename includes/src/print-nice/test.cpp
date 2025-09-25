#include <iostream>
#include "PrintStream.h"
#include "PrintNice.h"

int main() {

	PrintNice::print("Hello normal", OutputType::Normal);
	PrintNice::print("Hello success", OutputType::Success);
	PrintNice::print("Hello info", OutputType::Info);
	PrintNice::print("Hello warning", OutputType::Warning);
	PrintNice::print("Hello error", OutputType::Error);
	
	PrintNice::print();

	PrintNice::print("Bold", OutputType::Normal, TextStyle::Bold);
	PrintNice::print("Italic", OutputType::Normal, TextStyle::Italic);
	PrintNice::print("Underline", OutputType::Normal, TextStyle::Underline);

	PrintNice::print();

	PrintNice::error("Got an error, or maybe not, just a test!");
	PrintNice::error("Could be a bad error, instead it's just a test!", ErrorSeverity::Medium);
	PrintNice::error("Horrible error, oh wait, false alarm!", ErrorSeverity::High);

	PrintNice::print();

	PrintStream s = PrintNice::stream();
	s << "Lets stream something" << "and then some more";
	s.print();

	s <<
		"Let's mix things up a bit" <<
		TextStyledToken{
			"for example here is an underlined italic success",
			OutputType::Success,
			TextStyle::Italic | TextStyle::Underline
		} <<
		TextStyledToken{
			"even uses good stuff like StreamOut!",
			OutputType::Info,
			TextStyle::Bold
		} <<
	StreamOut();

	return 0;
}