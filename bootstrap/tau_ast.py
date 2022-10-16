from dataclasses import dataclass
from lark import ast_utils
from numpy import number
from typing import List

class ASTNode(ast_utils.Ast):
    attributes: object

    def visit(self, func):
        if func(self):
            members = [a for a in dir(self) if not a.startswith('_') and not callable(getattr(self, a))]
            for m in members:
                member = getattr(self,m)
                if isinstance(member, List):
                    for e in member:
                        if issubclass(e.__class__, ASTNode):
                            e.visit(func)
                else:
                    if issubclass(member.__class__, ASTNode):
                        member.visit(func)

@dataclass
class ID(ASTNode):
    name: str

@dataclass
class Compound(ASTNode):
    what: ASTNode

@dataclass
class Decoration(ASTNode):
    key: str
    value: object #StringLiteral, NumericLiteral or None

@dataclass
class Struct(ASTNode):
    id: ID
    members: List[ASTNode] # List(variable|embed)

@dataclass
class Variable(ASTNode):
    decorations: List[Decoration]
    typeID: ASTNode # ID or TypeDeclaration
    id: ID

@dataclass
class Embed(ASTNode):
    typeID: ID

@dataclass
class FunctionDeclaration(ASTNode):
    parameters: List[Variable] # List(variable)
    returnTypeID: ASTNode # ID or TypeDeclaration

@dataclass
class InterfaceFunction(ASTNode):
    id: ID
    declaration: FunctionDeclaration

@dataclass
class Interface(ASTNode):
    id: ID
    functions: List[InterfaceFunction]

@dataclass
class Function(ASTNode):
    id: ID
    parameters: List[Variable] # List(variable)
    returnTypeID: ID
    body: List[ASTNode]

@dataclass
class BinaryOperation(ASTNode):
    left: ASTNode
    right: ASTNode
    operation: str

@dataclass
class UnaryOperation(ASTNode):
    what: ASTNode
    operation: str

@dataclass
class LogicalOperation(ASTNode):
    left: ASTNode
    right: ASTNode
    operation: str

@dataclass
class GetItem(ASTNode):
    source: ASTNode
    subscript: ASTNode

@dataclass
class AssignOperation(ASTNode):
    left: ASTNode
    right: ASTNode

@dataclass
class SubscriptOffset(ASTNode):
    elements: number

@dataclass
class SubscriptSlice(ASTNode):
    start: number
    end: number

@dataclass
class SubscriptVariable(ASTNode):
    id: ID

@dataclass
class NumericLiteral(ASTNode):
    value: int

@dataclass
class StringLiteral(ASTNode):
    value: str

@dataclass
class GetAttribute(ASTNode):
    source: ASTNode
    what: ASTNode

@dataclass
class GetEnumValue(ASTNode):
    enum: ID
    value: ID

@dataclass
class MethodCall(ASTNode):
    source: ASTNode
    methodID: ASTNode
    parameters: List[ASTNode]

@dataclass
class FunctionCall(ASTNode):
    functionID: ASTNode
    parameters: List[ASTNode]

@dataclass
class ForLoop(ASTNode):
    init: List[ASTNode]
    test: ASTNode

@dataclass
class ForRange(ASTNode):
    iterator: ASTNode
    range: ASTNode

@dataclass
class For(ASTNode):
    loop: ASTNode
    statements: List[ASTNode]

@dataclass
class InitList(ASTNode):
    values: List[ASTNode]

@dataclass
class Return(ASTNode):
    result: ASTNode

@dataclass
class ConstantDeclaration(ASTNode):
    id: ID
    value: ASTNode

@dataclass
class Enum(ASTNode):
    id: ID
    ids: List[ID]

@dataclass
class TypeDeclaration(ASTNode):
    isConstant: object
    id: ID
    typelist: List[ASTNode]
    arraySize: int

@dataclass
class AliasDeclaration(ASTNode):
    decorations: List[Decoration]
    id: ID
    type: ASTNode #TypeDeclaration or FunctionDeclaration    

@dataclass
class ASTDecorator(ASTNode):
    id: ID
    sender: ID
    root: ID
    target: ID
    statements: List[ASTNode]

@dataclass
class Section(ASTNode):
    id: ID
    payload: object

@dataclass
class Symbol():
    libIndex: number
    name: str

@dataclass
class ImportSection():
    libraries: List[str]
    symbols: List[Symbol]

@dataclass
class Use(ASTNode):
    name: str

@dataclass
class Unit(ASTNode):
    module: str
    uses: List[Use]
    interfaces: List[Interface]
    structs: List[Struct]
    functions: List[Function]
    constants: List[ConstantDeclaration]
    aliases: List[AliasDeclaration]
    decorators: List[ASTDecorator]
    sections: List[Section]
    enums: List[Enum]

@dataclass
class Aliases(ASTNode):
    decorations: List[Decoration]
    aliases: List[AliasDeclaration]

@dataclass
class Constants(ASTNode):
    constants: List[ConstantDeclaration]

@dataclass
class If(ASTNode):
    test: ASTNode
    thenBlock: List[ASTNode]
    elseBlock: List[ASTNode]