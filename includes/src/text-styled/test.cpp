#include <iostream>

#include "TextStyled.h"

int main() {

	TextStyled t("Hello!");

	t.color("#00f06f");

	std::cout << t << std::endl;

	return 0;
}