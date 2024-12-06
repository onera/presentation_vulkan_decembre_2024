#ifndef _ANSI_ESCAPE_HPP_
#define _ANSI_ESCAPE_HPP_

namespace ansi
{
    constexpr char const* Normal            = "\033[0m";
    constexpr char const* Bright            = "\033[1m";
    constexpr char const* Underline         = "\033[4m";
    constexpr char const* Inverse           = "\033[7m";
    constexpr char const* PrimaryFont       = "\033[10m";
    constexpr char const* SecondFont        = "\033[11m";
    constexpr char const* ThirdFont         = "\033[12m";
    constexpr char const* FourthFont        = "\033[13m";
    constexpr char const* FifthFont         = "\033[14m";
    constexpr char const* SixthFont         = "\033[15m";
    constexpr char const* SeventhFont       = "\033[16m";
    constexpr char const* HeighthFont       = "\033[17m";
    constexpr char const* NinthFont         = "\033[18m";
    constexpr char const* TenthFont         = "\033[18m";
    constexpr char const* NormalIntensity   = "\033[22m";
    constexpr char const* NoUnderline       = "\033[24m";
    constexpr char const* Black             = "\033[30m";
    constexpr char const* Red               = "\033[31m";
    constexpr char const* Green             = "\033[32m";
    constexpr char const* Yellow            = "\033[33m";
    constexpr char const* Blue              = "\033[34m";
    constexpr char const* Magenta           = "\033[35m";
    constexpr char const* Cyan              = "\033[36m";
    constexpr char const* White             = "\033[37m";
    constexpr char const* DefaultColor      = "\033[49m";
    constexpr char const* BBlack            = "\033[40m";
    constexpr char const* BRed              = "\033[41m";
    constexpr char const* BGreen            = "\033[42m";
    constexpr char const* BYellow           = "\033[43m";
    constexpr char const* BBlue             = "\033[44m";
    constexpr char const* BMagenta          = "\033[45m";
    constexpr char const* BCyan             = "\033[46m";
    constexpr char const* BWhite            = "\033[47m";
    constexpr char const* DefaultBackground = "\033[49m";
    constexpr char const* Framed            = "\033[51m";
    constexpr char const* Encircled         = "\033[52m";
    constexpr char const* Overlined         = "\033[53m";
    constexpr char const* NoFramed          = "\033[54m";
}

#endif