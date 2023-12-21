/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f) // we define the CTRL_KEY(<something>) macro. It takes k and does a bitwise and with 0x1f (0b00011111)
/* In other words, it sets the upper 3 bits of the character to 0. 
This mirrors what the Ctrl key does in the terminal: it strips bits 5 and 6 from whatever key you press in combination with Ctrl, 
and sends that. (By convention, bit numbering starts from 0.) The ASCII character set seems to be designed this way on purpose. 
(It is also similarly designed so that you can set and clear bit 5 to switch between lowercase and uppercase.)*/



/*** data ***/
struct termios orig_termios;

/*** terminal ***/
void die(const char* s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s); 
    exit(1);} // error handling, perror() from stdio.h. perror() looks at the global errno variable and prints the error, as well as the string s to give context about where the error occurred
// any non zero exit() value means failure. exit() comes from stdlib.h
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) //The `TCSAFLUSH` argument specifies when to apply the change. see line 26
    // because of this leftover input is no longer fed to the terminal. TCSAFLUSH
    {
        die("tcserattr");
    }
}

void enableRawMode()
{
    struct termios raw; //create a termios structure called raw
    if(tcgetattr(STDIN_FILENO, &orig_termios) == -1) //get terminal attributes into raw
    {
        die("tcgetattr");
    }
    atexit(disableRawMode); //atexit() comes from <stdlib.h>.
    // We use it to register our disableRawMode() function to be called automatically 
    //when the program exits, whether it exits by returning from main(), or by calling the exit() function. This way we can ensure we’ll leave the terminal attributes the way we found them when our program exits.
    raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); //ICRNL is ['I' input flag, 'CR' is carriage return, and 'NL' is newline. stops coversion from carriage return (Ctrl - M or Enter {ASCII 13}) to newline(Ctrl - J or ASCII 10) ]| turns off the Ctrl - S and Ctrl - Q control signals (XOFF and XON)
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG); // setting the echo flag off so that the terminal does not echo whatever one types. canonical mode is also flipped off. now we read byte by byte. we exit as soon as 'q' is pressed.
    // .c_lflag field is for "local flags"
    /*ECHO is a bitflag, defined as 00000000000000000000000000001000 in binary. 
    We use the bitwise-NOT operator (~) on this value to get 11111111111111111111111111110111. 
    We then bitwise-AND this value with the flags field, which forces the fourth bit in the flags field to become 0, 
    and causes every other bit to retain its current value. 
    Flipping bits like this is common in C.*/
    raw.c_oflag &= ~(OPOST); // turns off output processing. POST is post processing of output. look up on \n to \r\n and carriagr returns and newlines
    raw.c_cflag |= (CS8); //character size 8 bits. this is a bit mask
    raw.c_cc[VMIN] = 0; // minimum bytes before read() can return
    raw.c_cc[VTIME] = 1; // time to wait before read() returns. time in 1/10 of a second
    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) // set the terminal attributes to those mentioned in raw
    /*The `TCSAFLUSH` argument specifies when to apply the change: 
    in this case, it waits for all pending output to be written to the terminal,
     and also discards any input that hasn’t been read.*/
     {
        die("tcsetattr");
     }
}

char editorReadKey() { // editorReadKey()’s job is to wait for one keypress, and return it.
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}
/*** output ***/

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // write 4 bytes to STD_FILENO. which 4 bytes? those in the second arg
    // \x1b is the escape character, ASCII 27 in decimal notation.
    /* \x1b[2J is an escape sequence. escape sequences start with \x1b[
        'J' command is for "Erase In Display". It clears the screen.
        Escape sequence commands take arguments, which come before the command. In this case the argument is 2, 
        which says to clear the entire screen. <esc>[1J would clear the screen up to where the cursor is, 
        and <esc>[0J would clear the screen from the cursor up to the end of the screen. 
        Also, 0 is the default argument for J, so just <esc>[J by itself would also clear the screen from 
        the cursor to the end.
    */
   write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/

void editorProcessKeypress() { // editorProcessKeypress() waits for a keypress, and then handles it.
    char c = editorReadKey();

    switch(c) {
        case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}

/*** init ***/
int main()
{
    enableRawMode();
    while(1)
    {
        editorProcessKeypress();
        // char c = '\0';
        // if(read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read"); // errno and EAGAIN from errno.h
        // if (iscntrl(c)) // comes from ctype.h and checks if the char is a control character (non-printable character) [ASCII 0-31 and 127 are control characters]
        // {
        //     printf("%d\r\n", c);
        // }
        // else
        // {
        //     printf("%d ('%c')\r\n", c, c);
        // }

        // if (c == CTRL_KEY('q')) break; // using the CTRL_KEY macro.
        
        
    }
    return 0;
}