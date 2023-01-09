from typing import List
from lark import Lark, Transformer, ast_utils, UnexpectedInput
import sys
import os
import pickle
from pathlib import Path
import yaml
from tau_ast import *
import zipfile_deflate64 as zipfile

this_module = sys.modules[__name__]

class TranformToNodes(Transformer):
    def then_block(self, s):
        return s
    def else_block(self, s):
        return s
    def aliases_(self, s):
        return Aliases(s[0],s[1:])
    def constants_(self, s):
        return Constants(s)
    def interfaces_(self, s):
        return Interface(s[0], s[1:])
    def NAME(self, s):
        return ID(str(s))
    def module(self, s):
        return s[0].name
    def arglist(self, s):
        return s
    def struct_body(self, s):
        return s
    def get_attr(self,s):
        return GetAttribute(s[0],s[1])
    def get_enum_value(self, s):
        return GetEnumValue(s[0],s[1])
    def assign(self, s):
        return AssignOperation(s[0],s[1])
    def IS_CONST(self,s):
        return True
    def IS_MUTATE(self, s):
        return False
    def variable_definition(self, s):
        typeDecl = TypeDeclaration(s[0],s[1][0],[] if s[1][1] == None else s[1][1], 0 if s[1][2] == None else s[1][2])
        initValue = s[3]
        if isinstance(s[3], InitDefault):
            initValue = FunctionCall(ID("init"+typeDecl.id.name),[])
        if isinstance(s[3], InitList):
            initValue = FunctionCall(ID("init"+typeDecl.id.name),s[3].values)
        return AssignOperation(Variable([],typeDecl,s[2]), initValue)
    def variable_declaration(self, s):
        return Variable([], s[0], s[1])
    def type_structure(self,s):
        return s
    def operator(self, s):
        if s[0].data == "operator_init":
            functionName:str = s[0].data.split('_')[1]
            parameters = []
            if len(s[1].parameters) > 1:
                parameters = s[1].parameters[1:]
            code = [s[1].parameters[0]]
            code.extend(s[2])
            code.append(Return(s[1].parameters[0].id))
            return Function(ID(functionName+s[1].parameters[0].typeID.id.name),FunctionDeclaration(parameters,s[1].parameters[0].typeID),code)
        if s[0].data == "operator_free":
            functionName:str = s[0].data.split('_')[1]
            return Function(ID(functionName+s[1].parameters[0].typeID.id.name),FunctionDeclaration(s[1].parameters,None),s[2])
        return None
    def for_declaration(self, s):
        return For(s[0],s[1:])
    def statements(self, s):
        return s
    def loop(self, s):
        return For(None, s)
    def if_declaration(self, s):
        return If(s[0],s[1],s[2])
    def paramlist(self, s):
        return s
    def typelist(self, s):
        return s
    def type_static_array(self, s):
        return int(s[0])
    def enum_(self, s):
        return Enum(s[0],s[1:])
    def type_declaration_(self, s):
        return TypeDeclaration(s[0], s[1][0], s[1][1] if s[1][1] != None else [], 0 if s[1][2] == None else s[1][2])
    def string(self, s):
        return StringLiteral(s[0])
    def number(self, s):
        return NumericLiteral(s[0])
    def DEC_NUMBER(self,s):
        return int(s)
    def SIGNED_DEC_NUMBER(self, s):
        return int(s)
    def HEX_NUMBER(self, s):
        return int(s,16)
    def STRING_LITERAL(self, s):
        return s[2:-2]
    def logic_or(self, s):
        if len(s) > 1:
            s[1].left = s[0]
            return LogicalOperation(None, s[1], "||")
        return LogicalOperation(None, s[0],"||")        
    def logic_and(self, s):
        if len(s) > 1:
            s[1].left = s[0]
            return LogicalOperation(None, s[1], "&&")
        return LogicalOperation(None, s[0],"&&")
    def larger_equal(self, s):
        return LogicalOperation(None, s[0],">=")
    def smaller_equal(self, s):
        return LogicalOperation(None, s[0],"<=")
    def unequal(self, s):
        return LogicalOperation(None, s[0],"!=")
    def equal(self, s):
        return LogicalOperation(None, s[0],"==")
    def sign_pos(self, s):
        return UnaryOperation(s[0], "+")
    def sign_neg(self, s):
        return UnaryOperation(s[0], "-")
    def bitwise_not(self, s):
        return UnaryOperation(s[0], "~")
    def logical_not(self, s):
        return UnaryOperation(s[0], "!")
    def prefix_inc(self, s):
        return UnaryOperation(s[0], "++&")
    def prefix_dec(self, s):
        return UnaryOperation(s[0], "--&")
    def postfix_inc(self, s):
        return UnaryOperation(s[0], "++")
    def postfix_dec(self, s):
        return UnaryOperation(s[0], "--")        
    def test_expr(self, s):
        s[1].left = s[0]
        return s[1]
    def arith_expr(self, s):        
        return BinaryOperation(s[0], s[1][1], s[1][0])
    def equality_expr(self, s):
        return LogicalOperation(s[0], s[1].right, s[1].operation)
    def relational_expr(self, s):
        return LogicalOperation(s[0], s[1].right, s[1].operation)
    def bit_equality_expr(self, s):
        return BinaryOperation(s[0], s[1][1], s[1][0])
    def bit_and(self, s):
        return ["&", s[0]]
    def bit_xor(self, s):
        return ["^", s[0]]
    def bit_or(self, s):
        return ["|", s[0]]
    def mul(self, s):        
        return ["*",s[0]]
    def add(self, s):
        return ["+",s[0]]
    def sub(self, s):
        return ["-",s[0]]
    def div(self, s):
        return ["/",s[0]]
    def mod(self, s):
        return ["%",s[0]]
    def compound(self, s):
        return Compound(s)
    def term(self, s):
        return BinaryOperation(s[0], s[1][1], s[1][0])
    def decorations(self, s):
        return s
    def unit_(self, s):
        uses = []
        interfaces = []
        structs = []
        functions = []
        constants = []
        aliases = []
        decorators = []
        sections = []
        enums = []
        for node in s[1:]:
            if isinstance(node, Use):
                uses.append(node)
            if isinstance(node, Struct):
                structs.append(node)
            if isinstance(node, Constants):
                for c in node.constants:
                    constants.append(c)
            if isinstance(node, Function):
                functions.append(node)
            if isinstance(node, Interface):
                interfaces.append(node)
            if isinstance(node, Aliases):
                for a in node.aliases:
                    if a.decorations == None:
                        a.decorations = []
                    if node.decorations != None:
                        a.decorations.extend(node.decorations)
                    aliases.append(a)
            if isinstance(node, ASTDecorator):
                for a in node.decorators:
                    decorators.append(a)
            if isinstance(node, Enum):
                enums.append(node)
        return Unit(s[0], uses, interfaces, structs, functions, constants, aliases, decorators, sections, enums)

def noParseTreeNodesLeft(node) -> bool:
    if issubclass(node.__class__, ASTNode):
        members = [a for a in dir(node) if not a.startswith('_') and not callable(getattr(node, a))]
        for m in members:
            member = getattr(node, m)
            if isinstance(member, List):
                for e in member:
                    if not issubclass(e.__class__, ASTNode):
                        print(e)
                        return False
            else:
                if not issubclass(m.__class__, ASTNode):
                    print(m)
                    return False
    return True

def parseTreeToTauAST(parseTree) -> ASTNode:
    transformer = ast_utils.create_transformer(this_module, TranformToNodes())
    ast = transformer.transform(parseTree)
    #ast.visit(noParseTreeNodesLeft)
    return ast

class Transformer:
    def bottom_up(self, node):
        result = node
        if issubclass(node.__class__, ASTNode):
            members = [a for a in dir(node) if not a.startswith('_') and not callable(getattr(node, a))]
            for m in members:
                member = getattr(node,m)
                if isinstance(member, List):
                    for e in member:
                        if issubclass(e.__class__, ASTNode):
                            e = self.bottom_up(e)
                else:
                    if issubclass(member.__class__, ASTNode):
                        setattr(node, m, self.bottom_up(member))
            result = getattr(self, node.__class__.__name__, self.__fallback__)(node)
        return result
    
    def __fallback__(self, node) -> ASTNode:
        return node

def AstTypeCheck(ast: Unit):
    pass

def ObtainLifetime(ast: Unit):
    pass

class LibDecorator():
    def run(self, sender:Decoration, root:Unit, target:AliasDeclaration):
        if sender.key.name == "lib" and isinstance(target.type, FunctionDeclaration):
            library = sender.value
            importSection = None
            # find the import section
            for section in root.sections:
                if section.id.name == "import":
                    importSection = section
                    break
            # if not exists then create one
            if importSection == None:
                importSection = Section(ID("import"), ImportSection([], []))
                root.sections.append(importSection)
            libraryIndex = None
            # find the library in the import section
            for lib in importSection.payload.libraries:
                if lib == library:
                    libraryIndex = importSection.payload.libraries.index(lib)
                    break
            # if not exists then create it
            if libraryIndex == None:
                importSection.payload.libraries.append(library)
                libraryIndex = importSection.payload.libraries.index(library)
            symbolIndex = None
            # find the symbol in the import section
            for symbol in importSection.payload.symbols:
                if symbol.libIndex == libraryIndex and symbol.name == target.id.name:
                    symbolIndex = importSection.payload.symbols.index(symbol)
                    break
            if symbolIndex == None:
                importSection.payload.symbols.append(Symbol(libraryIndex, target.id.name))           

class ASTDecorators():
    def __init__(self, ast: Unit) -> None:
        super().__init__()
        self.unit = ast
    
    def solve(self):
        for alias in self.unit.aliases:
            for decorator in alias.decorations:
                LibDecorator().run(decorator, self.unit, alias)

class ResolveType(Transformer):
    def __init__(self, typeID: ID, newTypeDeclaration: TypeDeclaration) -> None:
        super().__init__()
        self.typeID = typeID
        self.newTypeDeclaration = newTypeDeclaration

    def AliasDeclaration(self, node: AliasDeclaration):
        result = node
        if isinstance(result.type, TypeDeclaration):
            if result.type.id.name == self.typeID.name:
                result.type = self.newTypeDeclaration
        return result
    
    def FunctionDeclaration(self, node: FunctionDeclaration):
        result = node        
        if result.returnTypeID != None:
            if isinstance(result.returnTypeID, ID):
                if result.returnTypeID.name == self.typeID.name:
                    result.returnTypeID = self.newTypeDeclaration
            else:
                if result.returnTypeID.id.name == self.typeID.name:
                    result.returnTypeID = self.newTypeDeclaration
        return result

    def Variable(self, node: Variable):
        result = node        
        if isinstance(result.typeID, TypeDeclaration):
            if result.typeID.id.name == self.typeID.name:
                result.typeID = self.newTypeDeclaration
        else:
            if result.typeID.name == self.typeID.name:
                result.typeID = self.newTypeDeclaration
        return result

class ResolveConstantID(Transformer):
    def __init__(self, symbol: ID, replacement: ASTNode) -> None:
        super().__init__()
        self.symbol = symbol
        self.replacement = replacement

    def ID(self, node: ID):
        result = node
        if result.name == self.symbol.name:
            result = self.replacement
        return node

    def BinaryOperation(self, node: BinaryOperation):
        result = node
        if isinstance(result.right, ID):
            if result.right.name == self.symbol.name:
                result.right = self.replacement
        if isinstance(node.left, ID):
            if result.left.name == self.symbol.name:
                result.left = self.replacement
        return result

    def UnaryOperation(self, node:UnaryOperation):
        result = node
        if isinstance(result.what, ID):
            if result.what.name == self.symbol.name:
                result.what = self.replacement
        return result

    def AssignOperation(self, node:AssignOperation):
        result = node
        if isinstance(result.right, ID):
            if result.right.name == self.symbol.name:
                result.right = self.replacement
        return result

    def FunctionCall(self, node:FunctionCall):
        result = node
        for i in range(len(result.parameters)):
            p = result.parameters[i]
            if isinstance(p, ID):
                if p.name == self.symbol.name:
                    result.parameters[i] = self.replacement
        return result

class ResolveAliaseDeclaration():
    def __init__(self, ast: Unit) -> None:
        self.unit = ast
    
    def solveAliases(self):
        for a in self.unit.aliases:
            ResolveType(a.id, a.type).bottom_up(self.unit)

def solveConstants(ast: Unit):
    for c in ast.constants:
        resolver = ResolveConstantID(c.id, c.value)
        for f in ast.functions:
            resolver.bottom_up(f)

class ResolveCompiletimeExpressions(Transformer):
    def __init__(self) -> None:
        super().__init__()
        self.variables = {}
        #for c in ast.constants:
        #    self.variables[c.id.name] = c.value

    def BinaryOperation(self, node: BinaryOperation):
        result = node
        isLeftKnown = False
        isRightKnown = False
        if (isinstance(node.left, ID) and node.left.name in self.variables) or isinstance(node.left, NumericLiteral):
            isLeftKnown = True
        if (isinstance(node.right, ID) and node.right.name in self.variables) or isinstance(node.right, NumericLiteral):
            isRightKnown = True
        if isLeftKnown and isRightKnown:
            leftValue = None
            rightValue = None
            if isinstance(node.left, NumericLiteral):
                leftValue = node.left
            if isinstance(node.left, ID):
                leftValue = self.variables[node.left.name]
            if isinstance(node.right, NumericLiteral):
                rightValue = node.right
            if isinstance(node.right, ID):
                rightValue = self.variables[node.right.name]
            if isinstance(leftValue, NumericLiteral) and isinstance(rightValue, NumericLiteral):
                if node.operation == '*':
                    result = NumericLiteral(leftValue.value * rightValue.value)
                if node.operation == '+':
                    result = NumericLiteral(leftValue.value + rightValue.value)
                if node.operation == '-':
                    result = NumericLiteral(leftValue.value - rightValue.value)
                if node.operation == '/':
                    result = NumericLiteral(leftValue.value / rightValue.value)
        return result
    def UnaryOperation(self, node:UnaryOperation):
        if isinstance(node.what, NumericLiteral):
            if node.operation == '-':
                node.what.value = -abs(node.what.value)
            if node.operation == '+':
                node.what.value = abs(node.what.value)
            if node.operation == '~':
                node.what.value = -node.what.value
            return node.what
        return node
    def AssignOperation(self, node:AssignOperation):
        if isinstance(node.right, NumericLiteral) and isinstance(node.left, Variable):
            self.variables[node.left.id.name] = node.right
        return node

class StructSolver:
    def __init__(self, ast: Unit) -> None:
        self.unsolved = []
        self.known = {}
        self.ast = ast

    def solve(self) -> bool:
        # find unsolved structs
        for struct in self.ast.structs:
            isSolved = True
            for m in struct.members:
                if isinstance(m, Embed):
                    isSolved= False
                    break
            if isSolved:
                self.known[struct.id.name]=struct
            else:
                self.unsolved.append(struct)
        
        # resolve the unsolved structs
        noProgress = False
        while noProgress == False:
            noProgress = True
            for struct in self.unsolved:
                isSolved = True
                for m in struct.members:
                    if isinstance(m, Embed):
                        isSolved = False
                        if m.typeID.name in self.known:
                            index = struct.members.index(m)
                            for e in self.known[m.typeID.name].members:
                                struct.members.insert(index, e)
                            struct.members.remove(m)
                            noProgress = False
                if isSolved:
                    self.unsolved.remove(struct)
                    self.known[struct.id.name] = struct
        return True if len(self.unsolved) == 0 else False

def joinAST(first: Unit, second:Unit):
    if first.module == second.module:
        first.aliases.extend(second.aliases)
        first.constants.extend(second.constants)
        first.decorators.extend(second.decorators)
        first.functions.extend(second.functions)
        first.sections.extend(second.sections)
        first.structs.extend(second.structs)
        first.uses.extend(second.uses)
        first.enums.extend(second.enums)
        first.interfaces.extend(second.interfaces)

def loadDependencies(target: Unit, dep: str, module_directory: str):
    moduleFile = module_directory+dep+'.taum'
    with open(moduleFile, 'rb') as f:
        taum = pickle.loads(f.read())
        for use in taum.uses:
            loadDependencies(target, use.name, module_directory)
        #module.constants.extend(taum.constants)
        target.aliases.extend(taum.aliases)

def build(input, module_directory:str = 'tau/', cache_directory:str = 'tau/', tau_dir:str = './') -> str:
    grammar = None
    with open(os.path.dirname(__file__)+"/../resources/tau.lalr", 'r') as f:
        grammar = f.read()
    inputFiles = []
    if os.path.isfile(input):
        inputFiles.append(input)
    else:
        for root, dirs, files in os.walk(input):
            for file in files:
                if file.endswith(".tau"):
                    inputFiles.append(os.path.join(root,file))
                
    module = None
    for filename in inputFiles:
        print("Parse "+filename)
        parser = Lark(grammar, start='unit_', parser="lalr", maybe_placeholders=True)
        #print(parser.terminals)
        try:
            with open(filename) as f:
                parseTree = parser.parse(f.read())
                cachefile = cache_directory+filename+'-parsetree.txt'
                cachedir = os.path.dirname(cachefile)
                if not os.path.exists(cachedir):
                    os.makedirs(cachedir)
                try:
                    dbg = open(cachefile,'w')
                except FileNotFoundError:
                    dbg = open(cachefile,'a')
                dbg.write(parseTree.pretty())
                dbg.close()
                #print(parseTree)
                ast = parseTreeToTauAST(parseTree)
                cachefile = cache_directory+filename+'.ast'
                try:
                    ast_file = open(cachefile, 'wb')
                except FileNotFoundError:
                    ast_file = open(cachefile, 'ab')
                ast_file.write(pickle.dumps(ast))
                ast_file.close()
                if module == None:
                    module = ast
                else:
                    joinAST(module, ast)
        except UnexpectedInput as u:
            print('Parser error: '+filename)
            print(u)
            exit(1)

    if module != None:
        print("Process module AST: "+module.module)
        moduleAliases = module.aliases
        module.aliases = []
        module.aliases.extend(moduleAliases)
        for use in module.uses:
            loadDependencies(module, use.name, module_directory)
        #print(yaml.dump(module))
        # Run all decorators to process the decoration of all AST nodes.
        ASTDecorators(module).solve()
        # Resolve all alias to replace the type declarations by intrinsic types.
        ResolveAliaseDeclaration(module).solveAliases()
        module.aliases = moduleAliases
        # Resolve all constants.
        solveConstants(module)
        # Validate that all type declarations are matching.
        AstTypeCheck(module)
        #print(ast)
        # Process all embed instructions of the struct elements.
        solver = StructSolver(module)
        if solver.solve() == False:
            print("Error: Couldn't solve all embed instructions.")
            exit(1) 
        ObtainLifetime(module)
        # Process all operations which can be done at compile time.
        #ResolveCompiletimeExpressions().bottom_up(module)      
        # Generate module.
        moduleFile = module_directory+'/'+module.module+'.taum'
        moduleBytes = 0
        with open(moduleFile, 'wb') as f:
            moduleBytes = f.write(pickle.dumps(module))
        # Generate module package.
        modulePackage = module_directory+'/'+module.module+'.zip'
        yml = yaml.dump_all([
            {
                "apiVersion": "v1",
                "kind": "moduleInfo",
                "version": "1.0.0",
                "name": module.module
            },
            {
                "apiVersion": "v1",
                "kind": "ast",
                "format": "pickle",
                "data": {
                    "path": module.module+".taum",
                    "bytes": moduleBytes
                }
            }
        ])
        zip = zipfile.ZipFile(modulePackage, 'w', zipfile.ZIP_DEFLATED, True, 9)
        zip.writestr('module.yaml',yml)
        zip.write(moduleFile, module.module+'.taum')
        zip.close()
        return module.module+'.taum'
    else:
        return None

if __name__ == '__main__':
    module_dir = Path(os.getenv('APPDATA'))/"tau"/"modules"
    module_dir.mkdir(parents=True, exist_ok=True)
    module_directory = module_dir.as_posix()+'/'
    cache_dir = Path(os.getenv('APPDATA'))/"tau"/"cache"
    cache_dir.mkdir(parents=True, exist_ok=True)
    cache_directory = cache_dir.as_posix()+'/'
    build(sys.argv[1], module_directory, cache_directory)