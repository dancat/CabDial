#include <cassert>
#include <cstdint>

static constexpr uint8_t FUNCTIONS_PER_PAGE = 5;

static uint8_t pageCount(uint8_t availableFunctions)
{
    return (availableFunctions + FUNCTIONS_PER_PAGE - 1) / FUNCTIONS_PER_PAGE;
}

static uint8_t functionOnPage(const uint8_t *functions, uint8_t count,
                              uint8_t page, uint8_t slot)
{
    const uint8_t index = page * FUNCTIONS_PER_PAGE + slot;
    return index < count ? functions[index] : UINT8_MAX;
}

int main()
{
    const uint8_t defined[] = {0, 1, 4, 7, 8, 12, 17};
    assert(pageCount(7) == 2);
    assert(functionOnPage(defined, 7, 0, 0) == 0);
    assert(functionOnPage(defined, 7, 0, 4) == 8);
    assert(functionOnPage(defined, 7, 1, 0) == 12);
    assert(functionOnPage(defined, 7, 1, 1) == 17);
    assert(functionOnPage(defined, 7, 1, 2) == UINT8_MAX);
    assert(pageCount(5) == 1);
    assert(pageCount(0) == 0);
}
