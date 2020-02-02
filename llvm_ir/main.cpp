#include <llvm-c/Core.h>
#include <unistd.h>

int main(int argc, char const *argv[]) {
    dup2(1, 2); // redirect stderr to stdout

    LLVMModuleRef mod = LLVMModuleCreateWithName("my_module");
    LLVMBuilderRef builder = LLVMCreateBuilder();

    LLVMValueRef sum;
    {
        LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
        LLVMTypeRef ftype = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
        sum = LLVMAddFunction(mod, "sum", ftype);
        LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
        LLVMPositionBuilderAtEnd(builder, entry);
        LLVMValueRef tmp = LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
        LLVMBuildRet(builder, tmp);
    }

    LLVMTypeRef param1 =  LLVMPointerType(LLVMInt8Type(), 0);
    LLVMTypeRef printf_type = LLVMFunctionType(LLVMInt32Type(), &param1, 1, true);
    LLVMValueRef printf = LLVMAddFunction(mod, "printf", printf_type);

    //LLVMValueRef g_fmt = LLVMAddGlobal(mod, LLVMArrayType(LLVMInt8Type(), 4), "fmt");
    //LLVMSetInitializer(g_fmt, LLVMConstString("%d\n", 3, false));
    LLVMValueRef g_fmt = LLVMBuildGlobalStringPtr(builder, "%d\n", "g_fmt");


    LLVMTypeRef ftype = LLVMFunctionType(LLVMInt32Type(), nullptr, 0, false);
    LLVMValueRef main = LLVMAddFunction(mod, "main", ftype);
    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(main, "entry");
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef arg1 = LLVMConstInt(LLVMInt32Type(), 34, false);
    LLVMValueRef arg2 = LLVMConstInt(LLVMInt32Type(), 110, false);
    LLVMValueRef args[] = {arg1, arg2};
    LLVMValueRef result = LLVMBuildCall(builder, sum, args, 2, "result");

    LLVMValueRef indices[] = {LLVMConstInt(LLVMInt32Type(), 0, false)};
    LLVMValueRef ptr_g_fmt = LLVMBuildGEP(builder, g_fmt, indices, 1, "ptr_g_fmt");

    LLVMValueRef args2[] = {ptr_g_fmt, result};

    LLVMBuildCall(builder, printf, args2, 2, "result2");

    LLVMBuildRet(builder, result);

    LLVMDumpModule(mod);
}
