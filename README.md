# small-c-compiler

Practically identical C complier that is capable of compiling itself(hopefully in the future) and has most of the features of C up to C11.  
Currently, it generates 16-bit and 32-bit assembly code for gcc that can then be assembled and linked into Windows and Linux. But it cannot compile full programs yet.
Look at the test directory to see the functionality of what it is able to do. 

## Installation
- Clone the repository to your remote device through git
- Some sort of C Compiler already installed(GNU, Clang, etc.)
- make tool installed(this is in the build-essentials package on linux and can be installed with `sudo apt install build-essentials`)
- Make sure that whatever terminal you have has read and executable permissions on shell scripts
- On Linux, the command would be chmod r+x `filename`
- in this case, it would be `driver.sh`

## Usage
- `make test` and it should run all of the test files through a shell script
- Alternatively, to see the actual assembly output you can run `make main` and then run `./main -o tmp.s test/testfile.c`
- Then, the asm output will be stored in tmp.s and all you have to do is open it in some editor or in bash use `cat tmp.s`
- Feel free to change `testfile.c` and play around with it and see how it changes the asm output, of course not everything is implemented yet, but there's a good bit of C implemented already. 


### References:
- https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf  
- http://www.open-std.org/jtc1/sc22/wg14/www/docs/C99RationaleV5.10.pdf  
- https://craftinginterpreters.com/contents.html  
- https://www.sigbus.info/compilerbook  
- https://www.cs.cmu.edu/~rdriley/487/papers/Thompson_1984_ReflectionsonTrustingTrust.pdf  
