import typing


class Object:
    def __init__(self, name: str, object_type: str = ""):
        self.name = name
        self.type = object_type

    def __repr__(self) -> str:
        return f"{self.name} - {self.type}" if self.type else self.name


def create_inline_group(name: str, token: str):
    return f"({name} {token})"


def create_group(
    name: str, tokens: typing.List
) -> typing.List[typing.Union[str, typing.List]]:
    return [f"({name}", tokens, ")"]


Formula = typing.Union[str, typing.List]


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
    var = Object(f"?{var}", var_type)
    return create_group(f"forall {var}", formula)


def _exists(var: str, var_type: str, formula: Formula):
    var = Object(f"?{var}", var_type)
    return create_group(f"exists {var}", formula)


def _when(condition: Formula, implies: Formula):
    return create_group("when", condition, implies)


def parse_proposition(str_prop: str) -> typing.Tuple[str, typing.List[str]]:
    """Parses the head and arguments of a proposition string.

    For example, 'on(box, table)' becomes ('on', ['box', 'table']).
    """
    import re

    matches = re.match("([^\(]*)\(([^\)]*)", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse proposition from '{str_prop}'.")
    name_pred = matches.group(1)
    str_args = matches.group(2).replace(" ", "").split(",")
    return name_pred, str_args


def parse_head(str_prop: str) -> str:
    """Parses the head of a proposition string."""
    import re

    matches = re.match("([^\(]*)\([^\)]*", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse proposition from '{str_prop}'.")
    name_pred = matches.group(1)
    return name_pred


def parse_args(str_prop: str) -> typing.List[str]:
    """Parses the arguments of a proposition string."""
    import re

    matches = re.match("[^\(]*\(([^\)]*)", str_prop)
    if matches is None:
        raise ValueError(f"Unable to parse objects from '{str_prop}'.")
    str_args = matches.group(1).replace(" ", "").split(",")
    return str_args


class Problem:
    def __init__(self, name: str, domain: str = ""):
        self._name = name
        self._domain = domain

        self._objects: typing.List[Object] = []
        self._initial_state: typing.List[str] = []
        self._goal = "(and)"

    def add_object(self, name: str, object_type: str = ""):
        self._objects.append(Object(name, object_type))

    def add_initial_prop(self, prop: str):
        if prop and prop[0] != "(":
            head, args = parse_proposition(prop)
            prop = _P(head, *args)
        self._initial_state.append(prop)

    def set_initial_state(self, state: typing.Set[str]):
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
    def objects(self) -> typing.List[typing.Union[str, typing.List[str]]]:
        return create_group(":objects", map(str, self._objects))

    @property
    def initial_state(self) -> typing.List[typing.Union[str, typing.List[str]]]:
        return create_group(":init", self._initial_state)

    @property
    def goal(self) -> typing.List[typing.Union[str, Formula]]:
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
        def pprint(strings: typing.Union[str, typing.List], indent: int = 0) -> str:
            if isinstance(strings, str):
                return "\t" * (indent - 1) + strings + "\n"
            return "".join(pprint(s, indent + 1) for s in strings)

        return pprint(self.definition)
