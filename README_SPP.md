# llvm
llvm pass to obtain vulnerable buffer list and replace unsafe buffer by replacing malloc by new secure_malloc function

## To obtain officiel llvm source code that we used

git clone https://github.com/djadja/llvm-project.git

git branch danger

## To compile it

cd build/

cmake ../

# To register or compile a pass

cmake --build .

# For testing our pass

## Create the c file for testing your pass and compile it to obatain the bytecode

clang -emit-llvm -c hello.c

## Test the pass on this file

cd build/

If you use the legacy pass Manager add the option -enable-new-pm=0 to this command

./bin/opt -load lib/LLVMOurPass.so -OurPass <hello.bc> /dev/null

if you want to see the changes in the LLVM IR file, redirect the exit to a new bytecode file and diassemble it

./bin/opt -load lib/LLVMOurPass.so -OurPass <hello.bc> new.bc

llvm-dis new.bc

## Study of the CVE

Link : https://docs.google.com/document/d/12eHdHtS9a8MJASe1TlBE05Z324Ou-45J0y23o6YmYWQ/edit?usp=sharing


