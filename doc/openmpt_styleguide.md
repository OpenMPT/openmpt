
Coding conventions
------------------


### OpenMPT

**Note:**
**This applies to all source code *except* for `libopenmpt/` and `openmpt123/`**
**directories.**
**Use libopenmpt style otherwise.**

(see below for an example)

 *  Place curly braces at the beginning of the line, not at the end
 *  Generally make use of the custom index types like `SAMPLEINDEX` or
    `ORDERINDEX` when referring to samples, orders, etc.
 *  When changing playback behaviour, make sure that you use the function
    `CSoundFile::IsCompatibleMode()` so that modules made with previous versions
    of MPT still sound correct (if the change is extremely small, this might be
    unnecessary)
 *  `CamelCase` function and variable names are preferred.

#### OpenMPT code example

~~~~{.cpp}
void Foo::Bar(int foobar)
{
    while(true)
    {
        // some code
    }
}
~~~~

