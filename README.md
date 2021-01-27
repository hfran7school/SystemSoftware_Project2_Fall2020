# SystemSoftware_Project2_Fall2020
COP3404: Project2: Passes 1 and 2 -- Generating SIC Object File
 * Author: Hailey Francis (n01402670@unf.edu)
 * Last Edited: 10/8/20
 
I did not include all 10 test files we were given because I do not own them, however I included one to show
what my example test object was being generated from.

Project 2 is an extension of Project 1. Much of what was used for Pass 1 is from my Project 1.

I was marked off on this assignment because the extension for the object files is supposed to be ".sic.obj"
and not just ".obj". However, this is a quick fix and will provide good practice when I learn how to patch
files from Linux.

The following is the documentation I turned in to my professor upon completion of the project:

# Hailey Francis -- Documentation Project 2 

Project 1 actually had an issue with the hash table, but luckily I was able to fix it with no issues for pass 2.
There was also an issue with the tokenizing of the character constants (case BYTE C''). It tokenizing spaces spaces
were an important part of the BYTE constant. It took me multiple days, but I was able to solve this issue and I now
have a bit of a better understanding of strtok.

We only discussed two errors in class: Invalid opcode and Invalid operand. I had already accounted for invalid opcodes
in pass 1, so invalid operand checks happen within pass 2. There really weren't any other errors I could think of to
catch in pass 2. All other possible errors are caught within pass 1.

I rewrote the OPTAB to be an array of static Opcode structs that contained the opcode and the mneumonic, as opposed
to just static string constants.

In pass 1, I had written the symbols in the symbol table to an intermediate file. However, after days of confusion and
stress, I realized that the FILE pointers associated with that intermediate file were interfering somewhere and causing
segfaults when trying to open the input file again in pass_2. (I chose to have pass 1 and pass 2 as two seperate functions, 
which led me to closing and reopening the input file rather than rewinding.) Luckily, since it wasn't required, I was able to 
just remove all of the code associated with writing the intermediate file. I'm not 100% sure how to fix this in the future if 
I did need that intermediate file, however I'm just relieved I was able to find out what was wrong and move forward. 

I used fprintf to print the HEADER immediately to the objectFile. I have an objectCode array that I would append the parts of 
the object code to as it was being calculated, and I was seperately keeping track the object code's length. If the object code's 
length was > 63 (if it was 64 or greater and the next part of the object code was 6 characters it would be too large), or if RESB 
or RESW is encountered, it would print the length of the object code and then the object code itself to the object file, with a 
"newRecord" checking variable being changed so that the program would start a new Text Record. The object code array and length are 
cleared every time a record is printed so that it can be prepared to get the new object code. The end record is written when the END 
directive is encountered.

If a BYTE C'' constant is too big for the current text record, a new one will be created. This takes place within the check for 
BYTE C''. While there are no test files with this example, it will do the same for a BYTE X'' constant.

If an error is encountered in the assembly during pass_2, then the objectFile will be closed and deleted. When an error is encountered
in either pass, it will print the error to the terminal as well as which pass the error was found during.
