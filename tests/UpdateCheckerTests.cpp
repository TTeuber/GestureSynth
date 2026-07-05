#include "Utility/UpdateChecker.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Version comparison", "[updatecheck]")
{
    using UpdateCheck::isNewerVersion;

    SECTION ("equal versions are not newer")
    {
        CHECK_FALSE (isNewerVersion ("0.1.0", "0.1.0"));
        CHECK_FALSE (isNewerVersion ("1.2.3", "v1.2.3"));
    }

    SECTION ("newer versions are detected")
    {
        CHECK (isNewerVersion ("0.1.0", "0.1.1"));
        CHECK (isNewerVersion ("0.1.0", "0.2.0"));
        CHECK (isNewerVersion ("0.1.0", "1.0.0"));
        CHECK (isNewerVersion ("1.9.9", "2.0.0"));
    }

    SECTION ("older versions are not newer")
    {
        CHECK_FALSE (isNewerVersion ("0.1.1", "0.1.0"));
        CHECK_FALSE (isNewerVersion ("1.0.0", "0.9.9"));
    }

    SECTION ("comparison is numeric, not lexicographic")
    {
        CHECK (isNewerVersion ("0.9.0", "0.10.0"));
        CHECK_FALSE (isNewerVersion ("0.10.0", "0.9.0"));
    }

    SECTION ("tag prefixes and suffixes are tolerated")
    {
        CHECK (isNewerVersion ("0.1.0", "v0.2.0"));
        CHECK (isNewerVersion ("v0.1.0", "0.2.0"));
        CHECK (isNewerVersion ("0.1.0", "0.2.0-beta"));
    }

    SECTION ("short versions are padded with zeros")
    {
        CHECK (isNewerVersion ("1.2", "1.2.1"));
        CHECK_FALSE (isNewerVersion ("1.2.0", "1.2"));
        CHECK (isNewerVersion ("1", "1.0.1"));
    }

    SECTION ("garbage or empty latest version never triggers an update")
    {
        CHECK_FALSE (isNewerVersion ("0.1.0", ""));
        CHECK_FALSE (isNewerVersion ("0.1.0", "v"));
        CHECK_FALSE (isNewerVersion ("0.1.0", "not-a-version"));
    }
}
