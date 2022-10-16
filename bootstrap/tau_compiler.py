import shutil
from typing import List
import sys
import os
import pickle
from pathlib import Path
from tau_ast import *

class Visitor:
    def top_down(self, node):
        if issubclass(node.__class__, ASTNode):
            visit_childs = getattr(self, node.__class__.__name__, self.__fallback__)(node)
            if visit_childs == True:
                members = [a for a in dir(node) if not a.startswith('_') and not callable(getattr(node, a))]
                for m in members:
                    member = getattr(node,m)
                    if isinstance(member, List):
                        for e in member:
                            if issubclass(e.__class__, ASTNode):
                                self.top_down(e)
                    else:
                        if issubclass(member.__class__, ASTNode):
                            self.top_down(member)

    def bottom_up(self, node):
        if issubclass(node.__class__, ASTNode):
            members = [a for a in dir(node) if not a.startswith('_') and not callable(getattr(node, a))]
            for m in members:
                member = getattr(node,m)
                if isinstance(member, List):
                    for e in member:
                        if issubclass(e.__class__, ASTNode):
                            self.bottom_up(e)
                else:
                    if issubclass(member.__class__, ASTNode):
                        self.bottom_up(member)
            getattr(self, node.__class__.__name__, self.__fallback__)(node)

    def __fallback__(self, node):
        return True

class ASTToCpp(Visitor):
    def __init__(self) -> None:
        super().__init__()
        self.code = ""
        self.indent = 0
        self.templates = ['str', 'memory', 'allocator']
        self.templateIndex = 0
        self.typeMapping = {
            "auto":"auto",
            "i16":"int_fast16_t",
            "i32":"int_fast32_t",
            "u8":"uint8_t",
            "u16":"uint16_t",
            "u32":"uint_fast32_t",
            "u64":"uint_fast64_t",
            "ptr":"void*",
            "size":"size_t"
        }
        self.noSemicolon = False
    
    def solvePtr(self, typeID: TypeDeclaration)->str:
        result = ""
        ptrType = typeID.typelist[0]
        if ptrType.isConstant:
            result = "const "
        result += self.solveTypeDeclaration(ptrType)+"*"
        return result

    def solveTypeDeclaration(self, typeID: TypeDeclaration) -> str:
        disableTemplate = False
        # type mapping or just id
        if typeID.id.name in self.typeMapping:
            # ptr or other mapping
            if typeID.id.name == "ptr":
                typename = self.solvePtr(typeID)
                disableTemplate = True
            else:
                typename = self.typeMapping[typeID.id.name]
        else:
            typename = typeID.id.name
        # template
        if disableTemplate == False and len(typeID.typelist) > 0:
            typename += "<%s" % (self.solveTypeDeclaration(typeID.typelist[0]))
            for p in typeID.typelist[0:-1]:
                typename += ", %s" % (self.solveTypeDeclaration(p))
            typename += ">"
        # Array
        if typeID.arraySize > 0:
            typename = "arr<%s,%d>" % (typename, typeID.arraySize)
        if typename in self.templates:
            typename = 'T'+str(self.templateIndex)
            self.templateIndex += 1
        return typename

    def solveTypeID(self, typeID: ID) -> str:
        if typeID.name in self.typeMapping:
            typename = self.typeMapping[typeID.name]
        else:
            typename = typeID.name
        if typename in self.templates:
            typename = 'T'+str(self.templateIndex)
            self.templateIndex += 1
        return typename

    def renderFunctionParameter(self, params: List[Variable]) -> str:
        result = ""
        self.templateIndex = 0
        if params != None and len(params) > 0:
            for param in params[:-1]:
                if isinstance(param.typeID, TypeDeclaration):
                    result += '%s %s,' % (self.solveTypeDeclaration(param.typeID), param.id.name)
                else:
                    result += '%s %s,' % (self.solveTypeID(param.typeID.id), param.id.name)
            if isinstance(params[-1].typeID, TypeDeclaration):
                result += '%s %s' % (self.solveTypeDeclaration(params[-1].typeID), params[-1].id.name)
            else:
                result += '%s %s' % (self.solveTypeID(params[-1].typeID.id), params[-1].id.name)
        return result

    def renderTemplateList(self, parameters:List[TypeDeclaration]):
        result = ''
        i = 0
        for p in parameters:            
            if p.typeID.id.name in self.templates:                
                result += ',class T' + str(i)
                i=i+1
        if result != '':
            result = 'template<'+result[1:]+'>\n'
        return result

    def renderFunctionDeclaration(self, node:Function) -> str:
        result = ''
        returnType = "void" if node.returnTypeID == None else self.solveTypeID(node.returnTypeID) if isinstance(node.returnTypeID, ID) else self.solveTypeDeclaration(node.returnTypeID)
        params = self.renderFunctionParameter(node.parameters)
        result += '%s%s %s(%s)' % (self.renderTemplateList(node.parameters),returnType, node.id.name, params)
        return result

    def renderFunctionImport(self, name: str, decl:FunctionDeclaration):
        result = '__declspec(dllimport) '
        if isinstance(decl.returnTypeID, ID):
            returnType = "void" if decl.returnTypeID == None else self.solveTypeID(decl.returnTypeID)
        else:
            returnType = "void" if decl.returnTypeID == None else self.solveTypeDeclaration(decl.returnTypeID)
        params = self.renderFunctionParameter(decl.parameters)
        result += '%s %s(%s)' % (returnType, name, params)
        return result
    
    def renderEnumValues(self, node:Enum):
        result = ''
        if len(node.ids) > 0:
            result = node.ids[0].name
            for v in node.ids[1:]:
                result += ', '+v.name        
        return result

    def Unit(self, node:Unit) -> bool:
        self.code += '#pragma once\n// Constants\n'
        for const in node.constants:
            self.code += 'static const auto %s = %s;\n' % (const.id.name, const.value.value)
        for enum in node.enums:
            self.code += 'enum class %s{%s};\n' %(enum.id.name,self.renderEnumValues(enum))
        self.code += '// Uses\n'
        for use in node.uses:
            self.code += '#include "%s.cpp"\n' % (use.name)
        self.code += '// Forward declarations of all structs.\n'
        for struct in node.structs:
            self.code += 'struct %s;\n'% (struct.id.name)
#        self.code += '// Forward declarations of all interfaces.\n'
#        for interface in node.interfaces:
#            self.code += 'struct %s;\n'% (interface.id.name)
        self.code += '// Forward declarations of all functions.\n'
        importSection = None
        for section in node.sections:
            if section.id.name == "import":
                importSection = section
                break
        if importSection != None:
            self.code += '#ifdef __cplusplus\nextern "C" {\n #endif\n'
            for symbol in importSection.payload.symbols:
                for alias in node.aliases:
                    if alias.id.name == symbol.name:
                        self.code += self.renderFunctionImport(alias.id.name, alias.type) + ';\n'
            self.code += '#ifdef __cplusplus\n}\n#endif\n'
        for func in node.functions:
            self.code += self.renderFunctionDeclaration(func) + ';\n'
        self.code += '// Implementation of all structs.\n'
        for struct in node.structs:
            self.top_down(struct)
        self.code += '// Implementation of all interfaces.\n'
        for interface in node.interfaces:
            self.top_down(interface)
        self.code += '// Implementation of all functions.\n'
        for func in node.functions:
            self.top_down(func)
        return False

    def BinaryOperation(self, node:BinaryOperation) -> bool:
        print(node)
        self.top_down(node.left)
        self.code += node.operation
        self.top_down(node.right)
        return False

    def UnaryOperation(self, node:UnaryOperation) -> bool:
        if not node.operation in ["--","++"]:
            if node.operation in ["--&", "++&"]:
                self.code += node.operation[0:2]
            else:
                self.code += node.operation
        self.top_down(node.what)
        if node.operation in ["--","++"]:
            self.code += node.operation
        return False        

    def Struct(self, node:Struct) -> bool:
        self.code += 'struct %s {\n' % (node.id.name)
        for member in node.members:
            if isinstance(member, Variable):
                if isinstance(member.typeID, TypeDeclaration):
                    self.code += '  %s %s;\n' % (self.solveTypeDeclaration(member.typeID), member.id.name)
                else:
                    self.code += '  %s %s;\n' % (self.solveTypeID(member.typeID), member.id.name)                    
        self.code += "};\n"
        #self.code += '%s init%s(){%s result;return result;}\n' % (node.id.name,node.id.name,node.id.name)
        return False

    def Interface(self, node:Interface) -> bool:
        #for function in node.functions:
        #    if isinstance(function, InterfaceFunction):
        #        self.code += 'template<class T>\n%s %s(T& Self' % (self.solveTypeDeclaration(function.declaration.returnTypeID),function.id.name)
        #        if len(function.declaration.parameters) > 0:
        #            self.code += ',%s' % (self.renderFunctionParameter(function.declaration.parameters))
        #        self.code += '){return %s(Self);}\n' % (function.id.name)
        return False        

    def Function(self, node:Function)->bool:
        self.code += self.renderFunctionDeclaration(node) + ' {\n'
        self.indent += 1
        for e in node.body:
            self.code += "\t" * self.indent
            self.top_down(e)
            self.code += ";\n"
        self.indent -= 1
        self.code += '}\n'
        return False
    
    def AssignOperation(self, node:AssignOperation) -> bool:
        self.top_down(node.left)
        if node.right != None:
            self.code += '='
            if isinstance(node.left, Variable) and isinstance(node.left.typeID, TypeDeclaration) and len(node.left.typeID.typelist) > 0:
                node.right.attributes = node.left.typeID
            self.top_down(node.right)
        return False

    def GetAttribute(self, node:GetAttribute) -> bool:
        if isinstance(node.source, ID):
            self.code += '%s.%s' % (node.source.name, node.what.name)
        else:
            self.top_down(node.source)
            self.code += '.%s' % (node.what.name)            
        return False

    def For(self, node:For) -> bool:
        if isinstance(node.loop, ForRange):
            self.code += 'for(auto %s=%s;%s<%s;++%s)' % (node.loop.iterator.name,node.loop.range.start.value,node.loop.iterator.name,node.loop.range.end.name,node.loop.iterator.name)
        if node.loop == None:
            self.code += 'for(;;)'
        self.code += '{\n'
        for e in node.statements:
            self.code += "\t"
            self.top_down(e)
            self.code += ";\n"
        self.code += '}\n'
        return False

    def LogicalOperation(self, node:LogicalOperation) -> bool:
        self.top_down(node.left)
        self.code += ' %s ' % (node.operation)
        self.top_down(node.right)
        return False

    def If(self, node: If) -> bool:
        #print(node)
        self.code += 'if ('
        self.top_down(node.test)
        self.code += '){\n'
        self.indent += 1
        for stm in node.thenBlock:
            self.code += "\t" * self.indent
            self.top_down(stm)
            self.code += ";\n"
        self.indent -= 1
        if node.elseBlock != None:
            self.code += '\n\t} else {\n'
            self.indent += 1
            for stm in node.elseBlock:
                self.code += "\t" * self.indent
                self.top_down(stm)
                self.code += ";\n"
            self.indent -= 1
        self.code += '\n\t}\n'
        return False

    def SubscriptOffset(self, node:SubscriptOffset) -> bool:
        self.code += '[%d]' % (node.elements)
        return False

    def SubscriptVariable(self, node:SubscriptVariable) -> bool:
        self.code += '[%s]' % (node.id.name)
        return False

    def GetItem(self, node:GetItem) -> bool:
        self.top_down(node.source)
        self.top_down(node.subscript)
        return False

    def GetEnumValue(self, node:GetEnumValue) -> bool:
        self.code += '%s::%s' % (node.enum.name, node.value.name)
        return False

    def ID(self, node:ID) -> bool:
        self.code += node.name
        return False
    
    def Return(self, node:Return) ->bool:
        self.code += "return "
        self.top_down(node.result)
        return False
    
    def NumericLiteral(self, node:NumericLiteral) -> bool:
        self.code += str(node.value)
        return False

    def Compound(self, node:Compound) -> bool:
        self.code += "("
        self.top_down(node.what)
        self.code += ")"
        return False

    def StringLiteral(self, node:StringLiteral) -> bool:
        self.code += 'strlit{"'+node.value+'",'+str(len(node.value))+'}'
        return False

    def Variable(self, node:Variable) -> bool:
        if node.decorations != None:
            for decorator in node.decorations:
                if decorator.key.name == "thread":
                    self.code += "thread_local "
        if isinstance(node.typeID, TypeDeclaration):
            self.code += self.solveTypeDeclaration(node.typeID) + ' ' + node.id.name
        else:
            self.code += self.solveTypeID(node.typeID) + ' ' + node.id.name
        return False

    def InitList(self, node:InitList) -> bool:
        self.code += '{'
        for e in node.values[:-1]:
            self.top_down(e)
            self.code += ', '
        if len(node.values) > 0:
            self.top_down(node.values[-1])
        self.code += '}'
        return False

    def MethodCall(self, node:MethodCall) -> bool:
        self.code += '%s('%(node.methodID.name)
        self.noSemicolon = True
        self.top_down(node.source)
        if node.parameters != None and len(node.parameters) > 0:
            for param in node.parameters:
                self.code += ', '
                self.noSemicolon = True
                self.top_down(param)
        self.noSemicolon = False
        self.code += ')'
        return False

    def FunctionCall(self, node:FunctionCall) -> bool:
        if hasattr(node, 'attributes') and isinstance(node.attributes, TypeDeclaration):
            typelist = "%s" % (self.solveTypeDeclaration(node.attributes.typelist[0]))
            for p in node.attributes.typelist[0:-1]:
                typelist += ", %s" % (self.solveTypeDeclaration(p))
            self.code += '%s<%s>(' % (node.functionID.name, typelist)
        else:
            self.code += '%s('%(node.functionID.name)
        if node.parameters != None and len(node.parameters) > 0:
            for param in node.parameters[:-1]:
                self.noSemicolon = True
                self.top_down(param)
                self.code += ', '      
            self.noSemicolon = True      
            self.top_down(node.parameters[-1])
            self.noSemicolon = False
        self.code += ')'        
        return False

    def adjustAST(self, unit:ASTNode):
        if isinstance(unit, Unit):
            # generate all missing init functions
            for struct in unit.structs:
                initFunction = 'init'+struct.id.name
                found = False
                for function in unit.functions:
                    if initFunction == function.id.name:
                        found = True
                        break
                if found == False:
                    code = [
                        Variable(None, struct.id, ID('result')), 
                        Return(ID('result'))
                    ]
                    unit.functions.append(Function(ID(initFunction), [], struct.id, code))

def compile(unit: Unit, f, tau_directory:str) -> bool:
    envScript = r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
    compiler_options="/MT /Zc:inline /GF /GL /GS- /sdl- /EHa- /EHs-c- /D_HAS_EXCEPTIONS=0 /O2 /W3 /std:c++latest"
    linker_options="/nologo /NODEFAULTLIB /MANIFEST:NO /DYNAMICBASE:NO /FIXED /EMITPOGOPHASEINFO /emittoolversioninfo:no /ALIGN:16 /stub:\"%s\" \"kernel32.lib\"" %(tau_directory+'backend/vc++/msdos_stub.exe')
    print(os.popen("\""+envScript+"\" & \""+"cl.exe"+"\" "+compiler_options+" /I \gc_cache "+f+" /link "+linker_options+" /entry:run /SUBSYSTEM:CONSOLE").read())
    # debug
    compiler_options="/MTd /Zi /GF /GL /GS- /sdl- /EHa- /EHs-c- /D_HAS_EXCEPTIONS=0 /W3 /std:c++latest"
    fout = Path(f).name[:-4]+"d.exe"
    print(os.popen("\""+envScript+"\" & \""+"cl.exe"+"\" "+compiler_options+" /I \gc_cache "+f+" /Fe:"+fout+" /link "+linker_options+" /entry:run /SUBSYSTEM:CONSOLE").read())
    return True

def build(input, module_directory:str = 'tau/', cache_directory:str = 'tau/', tau_directory:str = './',  linkModule:str = None):
    module = None
    print(input)
    with open(module_directory+input, 'rb') as f:
        module = pickle.loads(f.read())
    # Generate the C++ code.
    cpp=ASTToCpp()
    cpp.adjustAST(module)
    cpp.top_down(module)
    if linkModule != None:
        code = '#define __PRE\n#include "builtin.cpp"\n#undef __PRE\n'+cpp.code+'#define __POST\n#include "builtin.cpp"\n#undef __POST'
    else:
        code = cpp.code
    codefile = cache_directory+module.module+'.cpp'
    with open(codefile, 'w') as f:
        f.write(code)

    # Compile the C++ code.
    if linkModule != None:
        cpp_stub = tau_directory+'backend/vc++/builtin.cpp'
        shutil.copy(cpp_stub, cache_directory)
        compile(module, linkModule, tau_directory)

if __name__ == '__main__':
    linkModule = None
    module_dir = Path(os.getenv('APPDATA'))/"tau"/"modules"
    module_dir.mkdir(parents=True, exist_ok=True)
    module_directory = module_dir.as_posix()+'/'
    cache_dir = Path(os.getenv('APPDATA'))/"tau"/"cache"
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_directory = cache_dir.as_posix()+'/'
    tau_dir = Path(os.curdir)
    tau_directory = tau_dir.as_posix()+'/'
    for arg in sys.argv:
        if arg.startswith("-link="):
            linkModule = cache_directory+arg[6:]+'.cpp'
    build(sys.argv[1], module_directory, cache_directory, tau_dir, linkModule)