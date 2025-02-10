#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <iostream>
#include <fstream>
#include <stack>

using namespace llvm;

int main() {
    LLVMContext context;
    Module module("HelloModule", context);
    IRBuilder<> builder(context);

    std::ifstream file("source.bf", std::ios::in);
    if (!file) {
        std::cerr << "Error opening file!" << std::endl;
        return 1;
    }

    Type *int32Type = Type::getInt32Ty(context);
    Type *int8Type = Type::getInt8Ty(context);

    // Define the main function type and create the function
    FunctionType *mainType = FunctionType::get(builder.getInt32Ty(), false);
    Function *mainFunc = Function::Create(mainType, Function::ExternalLinkage, "main", module);

    // Define putchar and getchar functions
    FunctionType *putcharType = FunctionType::get(Type::getInt32Ty(context), {Type::getInt32Ty(context)}, false);
    Function *putcharFunc = Function::Create(putcharType, Function::ExternalLinkage, "putchar", module);

    FunctionType *getcharType = FunctionType::get(Type::getInt32Ty(context), {}, false);
    Function *getcharFunc = Function::Create(getcharType, Function::ExternalLinkage, "getchar", module);

    // Define calloc function
    FunctionType *callocType = FunctionType::get(Type::getInt8PtrTy(context), {Type::getInt32Ty(context), Type::getInt32Ty(context)}, false);
    Function *callocFunc = Function::Create(callocType, Function::ExternalLinkage, "calloc", module);

    // Create the entry basic block and set the insert point
    BasicBlock *entryBB = BasicBlock::Create(context, "entry", mainFunc);
    builder.SetInsertPoint(entryBB);

    // Allocate memory for cells and cell index
    Value *cellIndexPtr = builder.CreateAlloca(int32Type, nullptr);
    Value *cellsPtr = builder.CreateAlloca(int8Type, ConstantInt::get(int32Type, 4096));
    builder.CreateStore(ConstantInt::get(int32Type, 0), cellIndexPtr);

    // Call calloc to allocate memory for cells
    cellsPtr = builder.CreateCall(callocFunc, {ConstantInt::get(context, APInt(32, 4096)), ConstantInt::get(context, APInt(32, 1))}, "cells");

    // Stack to keep track of loop start and end blocks
    std::stack<std::pair<BasicBlock *, BasicBlock *>> stk;

    char ch;
    while (file.get(ch)) {
        if (ch == '>') {
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *newVal = builder.CreateAdd(currentVal, ConstantInt::get(int32Type, 1));
            builder.CreateStore(newVal, cellIndexPtr);
        } 
        else if (ch == '<') {
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *newVal = builder.CreateSub(currentVal, ConstantInt::get(int32Type, 1));
            builder.CreateStore(newVal, cellIndexPtr);
        } 
        else if (ch == '+') {
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *currentAddr = builder.CreateGEP(int8Type, cellsPtr, currentVal);
            currentVal = builder.CreateLoad(int8Type, currentAddr);
            Value *newVal = builder.CreateAdd(currentVal, ConstantInt::get(int8Type, 1));
            builder.CreateStore(newVal, currentAddr);
        } 
        else if (ch == '-') {
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *currentAddr = builder.CreateGEP(int8Type, cellsPtr, currentVal);
            currentVal = builder.CreateLoad(int8Type, currentAddr);
            Value *newVal = builder.CreateSub(currentVal, ConstantInt::get(int8Type, 1));
            builder.CreateStore(newVal, currentAddr);
        } 
        else if (ch == '.') {
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *currentAddr = builder.CreateGEP(int8Type, cellsPtr, currentVal);
            currentVal = builder.CreateLoad(int8Type, currentAddr);
            Value *extendedVal = builder.CreateZExt(currentVal, int32Type);
            builder.CreateCall(putcharFunc, {extendedVal});
        } 
        else if (ch == ',') {
            Value *int32Character = builder.CreateCall(getcharFunc, {});
            Value *int8Character = builder.CreateTrunc(int32Character, int8Type);
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *currentAddr = builder.CreateGEP(int8Type, cellsPtr, currentVal);
            builder.CreateStore(int8Character, currentAddr);
        } 
        else if (ch == '[') {
            BasicBlock *loopHeader = BasicBlock::Create(context, "loop_header", mainFunc);
            BasicBlock *loopBody = BasicBlock::Create(context, "loop_body", mainFunc);
            BasicBlock *loopAfter = BasicBlock::Create(context, "loop_after", mainFunc);
            stk.push({loopHeader, loopAfter});

            builder.CreateBr(loopHeader);
            builder.SetInsertPoint(loopHeader);
            Value *currentVal = builder.CreateLoad(int32Type, cellIndexPtr);
            Value *currentAddr = builder.CreateGEP(int8Type, cellsPtr, currentVal);
            currentVal = builder.CreateLoad(int8Type, currentAddr);
            Value *condition = builder.CreateICmpEQ(currentVal, ConstantInt::get(int8Type, 0));
            builder.CreateCondBr(condition, loopAfter, loopBody);
            builder.SetInsertPoint(loopBody);
        } 
        else if (ch == ']') {
            auto [loopHeader, loopAfter] = stk.top();
            stk.pop();
            builder.CreateBr(loopHeader);
            builder.SetInsertPoint(loopAfter);
        } 
        else {
            // Handle invalid character (if necessary)
        }
    }

    // Return statement for the main function
    builder.CreateRet(builder.getInt32(0));

    // Verify module
    if (verifyModule(module, &errs())) {
        std::cerr << "Module verification failed!" << std::endl;
        return 1;
    }

    // Write LLVM IR to file
    std::error_code ec;
    raw_fd_ostream irFile("output.ll", ec, sys::fs::OF_None);
    if (ec) {
        errs() << "Could not open file: " << ec.message() << "\n";
        return 1;
    }
    module.print(irFile, nullptr);
    irFile.close();

    return 0;
}
