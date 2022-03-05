# pep8-simulator
Usage:
  pep8.exe [args]
    Args:
      -cf   Command file flag.  -cf followed by file name.  REQUIRED
      -if   Input file flag.  -if followed by file name.  Optional
      -i    Input flag. -i followed by a input string.  Optional
      -d    Turn on debug mode.  Provides more debugging output. Optional. If omitted, debugging mode is off
      -s    Turn on silent mode.  Turn off Register/Status dump after each command. If omitted, silent mode is off
      -dm   Dump memory flag.  -dm followed by a string with memory start address and end address separated by a space. Optional. If included, the memory from start to end will be dumped to screen when the program terminates.
    
    Example:"
    C:\pep8> ./pep8.exe -cf cmds.pepo -d -i "ABCD1234" -dm "25 50"
