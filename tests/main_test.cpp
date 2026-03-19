/*
    DAW - An open-source digital audio workstation
    Copyright (C) 2026 DAW Project Contributors

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Project builds and links correctly", "[smoke]")
{
    REQUIRE(true);
}

TEST_CASE("C++20 features are available", "[smoke]")
{
    auto lambda = []<typename T>(T value) { return value; };
    REQUIRE(lambda(42) == 42);
}
