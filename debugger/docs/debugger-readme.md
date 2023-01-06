# Must Have Features

Display source
 * Now
 	* Disassemble code to opcodes and instructions
 	* Extract labels
 * Later
 	* Actually pair with source code via listing file
 	* Edit to include comments?

Display registers
 * Now
 	* A,X,Y, SP, PC, Flags
 * Later
 	* IR, temp_addr, bus lines

Display raw memory
 * Now
 	* As hex bytes
 	* With address offsets
 	* From arbitrary address
 * Later
 	* As disassembled code
 	* As ints, longs etc. 

Single step forward through code
Run continuously
 * With pause/break button
 
Set breakpoint


# Display source
## Disassemble code to opcodes and instructions
We need to be able to list:
* address
* opcode
* args

We should highlight the line where the PC is

So we need to map PC to line of text

We need to be able to scroll. For now we'll load the entire text into the window

We need to disassemble a block on code (bytes) into 
* Addr
* Opcode (with meta)
* Args
And maintain a map of PC to line

No word wrapping in the view
Shrink spacing with view width
Colour highlighting?

Go to a particular address?




