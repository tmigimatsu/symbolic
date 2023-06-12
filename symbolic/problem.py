from typing import List, Union, Sequence, Set, Tuple


class Object:
    def __init__(self, name: str, object_type: str = ""):
        self.name = name
        self.type = object_type

    def __repr__(self) -> str:
        return f"{self.name} - {self.type}" if self.type else self.name

    def __hash__(self):
        return hash((self.name, self.type))

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, Object):
            return NotImplemented
        return self.name == other.name and self.type == other.type


def create_inline_group(name: str, token: str):
    return f"({name} {token})"


Formula = Union[str, Sequence]


def create_group(name: str, tokens: Sequence) -> List[Formula]:
    return [f"({name}", tokens, ")"]


def _P(predicate: str, *arguments: str):
    return create_inline_group(predicate, " ".join(arguments))


def _and(*formulas: Formula):
    return create_group("and", formulas)


def _or(*formulas: Formula):
    return create_group("or", formulas)


def _not(formulas: Formula):
    if isinstance(formulas, str):
        return create_inline_group("not", formulas)
    return create_group("not", formulas)


def _forall(var: str, var_type: str, formula: Formula):
    var_obj = Object(f"?{var}", var_type)
    return create_group(f"forall {var_obj}", formula)


def _exists(var: str, var_type: str, formula: Formula):
    var_obj = Object(f"?{var}", var_type)
    return create_group(f"exists {var_obj}", formula)


def _when(condition: Formula, implies: Formula):
    return create_group("when", (condition, implies))


def parse_proposition(str_prop: str) -> Tuple[str, List[str]]:
    """Parses the head and arguments of a proposition string.

    For example, 'on(box, table)' becomes ('on', ['box', 'table']).
    """
    import re

    matches = re.match(r"([^\(]*)\(([^\)]*)", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse proposition from '{str_prop}'.")
    name_pred = matches.group(1)
    str_args = matches.group(2).replace(" ", "").split(",")
    return name_pred, str_args


def parse_head(str_prop: str) -> str:
    """Parses the head of a proposition string."""
    import re

    matches = re.match(r"([^\(]*)\([^\)]*", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse proposition from '{str_prop}'.")
    name_pred = matches.group(1)
    return name_pred


def parse_args(str_prop: str) -> List[str]:
    """Parses the arguments of a proposition string."""
    import re

    matches = re.match(r"[^\(]*\(([^\)]*)", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse objects from '{str_prop}'.")
    str_args = matches.group(1).replace(" ", "").split(",")
    return str_args


def pprint_formula(strings: Union[str, List], indent: int = 0) -> str:
    if isinstance(strings, str):
        return "\t" * (indent - 1) + strings + "\n"
    return "".join(pprint_formula(s, indent + 1) for s in strings)


class Problem:
    def __init__(self, name: str, domain: str = ""):
        self._name = name
        self._domain = domain

        self._objects: Set[Object] = set()
        self._initial_state: Set[Object] = set()
        self._goal: Formula = "(and)"

    def add_object(self, name: str, object_type: str = "") -> bool:
        if Object(name, object_type) in self._objects:
            return False
        else:
            self._objects.add(Object(name, object_type))
            return True

    def add_initial_prop(self, prop: str) -> bool:
        if prop and prop[0] != "(":
            head, args = parse_proposition(prop)
            prop = _P(head, *args)
        if prop in self._initial_state:
            return False
        else:
            self._initial_state.add(prop)
            return True

    def set_initial_state(self, state: Set[str]):
        for prop in state:
            self.add_initial_prop(prop)

    def set_goal(self, goal: Formula):
        self._goal = goal

    @property
    def problem(self) -> str:
        return create_inline_group("problem", self._name)

    @property
    def domain(self) -> str:
        return create_inline_group(":domain", self._domain)

    @property
    def objects(self) -> List[Union[str, Sequence[str]]]:
        sorted_objects = sorted(self._objects, key=lambda x: x.name)
        return create_group(":objects", list(map(str, sorted_objects)))

    @property
    def initial_state(self) -> List[Union[str, Sequence[str]]]:
        sorted_state = sorted(self._initial_state)
        return create_group(":init", sorted_state)

    @property
    def goal(self) -> List[Union[str, Formula]]:
        if isinstance(self._goal, str):
            return [create_inline_group(":goal", self._goal)]
        return create_group(":goal", self._goal)

    @property
    def definition(self):
        return create_group(
            f"define {self.problem}",
            [self.domain, *self.objects, *self.initial_state, *self.goal],
        )

    def __repr__(self) -> str:
        return pprint_formula(self.definition)
