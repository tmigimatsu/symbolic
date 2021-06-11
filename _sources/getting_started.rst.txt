:tocdepth: 2

Getting Started
===============

Installation
------------

.. _Github Page: https://github.com/tmigimatsu/symbolic

See the `GitHub page`_ for installation instructions.

Loading PDDL
------------

PDDL specifications get loaded into a ``Pddl`` object. From here, you can list
the environment objects, apply actions to states, perform basic planning, etc.

C++
^^^

.. code-block:: cpp

   #include <symbolic/symbolic>

   int main(int argc, char* argv[]) {
     // Load PDDL.
     symbolic::Pddl pddl("resources/domain.pddl", "resources/problem.pddl");

     // Validate PDDL.
     pddl.IsValid(/*verbose=*/true);

     return 0;
   }

See the C++ :symbolic:`symbolic::Pddl` reference for more details.

Python
^^^^^^

.. code-block:: python

   import symbolic

   # Load PDDL.
   pddl = symbolic.Pddl("resources/domain.pddl", "resources/problem.pddl")

   # Validate PDDL.
   pddl.is_valid(verbose=True)

See the Python :class:`symbolic.Pddl` reference for more details.

States
------

Symbolic states are represented as sets of propositions that are true using the
closed-world assumption. This means that propositions not included in the set
are assumed to be false.

C++
^^^

The ``State`` class can be instantiated in numerous ways. The easiest is to treat
it as a set of proposition strings.

.. code-block:: cpp

   symbolic::State s(pddl, {"on(box, table)", "inworkspace(table)"});

Or using explicit types:

.. code-block:: cpp

   symbolic::Object box(pddl, "box");
   symbolic::Object table(pddl, "table");
   symbolic::Proposition on_box_table("on", {box, table});
   symbolic::Proposition inworkspace_table("inworkspace", {table});
   symbolic::State s = {on_box_table, inworkspace_table};

See the C++ :symbolic:`symbolic::State` reference for more details.

Python
^^^^^^

In Python, states are simply represented as sets of proposition strings.

.. code-block:: python

   s = {"on(box, table)", "inworkspace(table)"}

Actions
-------

Actions whose pre-conditions are satisfied can modify states according to their
post-conditions. Action calls (actions whose parameters are instantiated with
actual objects) are represented as strings.

C++
^^^

.. code-block:: cpp

   symbolic::State s(pddl, {"on(box, table)", "inworkspace(box)", "inworkspace(table)"});
   std::string action_call = "pick(box, table)";

   // Check pre-conditions.
   if (pddl.IsValidAction(s, action_call)) {
       // Apply post-conditions.
       symbolic::State s_next = pddl.NextState(s, action_call);
   }

Or using explicit types:

.. code-block:: cpp

   // Set up state.
   symbolic::Object box(pddl, "box");
   symbolic::Object table(pddl, "table");
   symbolic::Proposition on_box_table("on", {box, table});
   symbolic::Proposition inworkspace_table("inworkspace", {table});
   symbolic::State s = {on_box_table, inworkspace_table};

   // Set up action.
   symbolic::Action action(pddl, "pick");
   std::vector<symbolic::Object> args = {box, table};

   // Check pre-conditions.
   if (action.IsValid(s, args)) {
     // Apply post-conditions.
     symbolic::State s_next = action.Apply(s, args);
   }

See the C++ :symbolic:`symbolic::Pddl::NextState` and
:symbolic:`symbolic::Action` references for more details.

Python
^^^^^^

.. code-block:: python

   s = {"on(box, table)", "inworkspace(box)", "inworkspace(table)"}
   action_call = "pick(box, table)"

   # Check pre-conditions.
   if pddl.is_valid_action(s, action_call):
       # Apply post-conditions.
       s_next = pddl.next_state(s, action_call)

See the Python :func:`symbolic.Pddl.next_state` and :class:`symbolic.Action`
references for more details.

Goals
-----

PDDL goals are specified as first-order logic formulas. Checking if a state
satisfies the goal condition is simple:

C++
^^^

.. code-block:: cpp

   symbolic::State s = pddl.initial_state();
   if (pddl.IsGoalSatisfied(s)) {
     std::cout << "Done!" << std::endl;
   }

See the C++ :symbolic:`symbolic::Pddl` reference for more details.

Python
^^^^^^

.. code-block:: python

   s = pddl.initial_state
   if pddl.is_goal_satisfied(s):
       print("Done!")

See the Python :class:`symbolic.Pddl` reference for more details.

Planning
--------

``symbolic`` provides a breadth first search planner for planning in simple
domains.

C++
^^^

.. code-block:: cpp

   #include <symbolic/symbolic>

   int main(int argc, char* argv[]) {
     // Load PDDL.
     symbolic::Pddl pddl("resources/domain.pddl", "resources/problem.pddl");

     // Initialize basic forward planner from the PDDL initial state.
     symbolic::Planner planner(pddl);
     symbolic::BreadthFirstSearch bfs(planner.root(), args.depth, /*verbose=*/false);

     // Perform BFS until the first valid plan.
     std::cout << "Planning..." << std::endl;
     std::vector<symbolic::Planner::Node> plan = *bfs.begin();

     // Extract list of actions to execute.
     // The first nodes in plans returned by BFS just contain the initial state (no action).
     std::vector<std::string> action_skeleton;
     action_skeleton.reserve(plan.size() - 1);
     for (size_t i = 1; i < plan.size(); i++) {
       action_skeleton.push_back(plan[i].action());
     }

     // Execute plan.
     std::cout << "Executing plan..." << std::endl;
     symbolic::State s = pddl.initial_state();
     for (const std::string& a : action_skeleton) {
       s = pddl.NextState(s, a);
     }
     std::cout << "Final state: " << s << std::endl;
     std::cout << "Is goal satisfied? " << pddl.IsGoalSatisfied(s) << std::endl;

     // Find all valid plans.
     std::cout << "Planning..." << std::endl;
     size_t idx_plan = 0;
     for (const std::vector<symbolic::Planner::Node>& plan : bfs) {
       std::cout << "Solution " << idx_plan << std::endl
                 << "===========" << std::endl;

       // Iterate over all nodes in the plan.
       for (const symbolic::Planner::Node& node : plan) {
         std::cout << node << std::endl;
       }
       idx_plan++;
     }

     return 0;
   }

See the C++ :symbolic:`symbolic::Planner` and
:symbolic:`symbolic::BreadthFirstSearch` references for more details.

Python
^^^^^^

.. code-block:: python

   import symbolic

   # Load PDDL.
   pddl = symbolic.Pddl("resources/domain.pddl", "resources/problem.pddl")

   # Initialize basic forward planner from the PDDL initial state..
   planner = symbolic.Planner(pddl)
   bfs = symbolic.BreadthFirstSearch(planner.root, max_depth=5, verbose=False)

   # Perform BFS until the first valid plan.
   print("Planning...")
   plan = next(iter(bfs))

   # Extract list of actions to execute.
   # The first nodes in plans returned by BFS just contain the initial state (no action).
   action_skeleton = [node.action for node in plan[1:]]

   # Execute plan.
   print("Executing plan...")
   s = pddl.initial_state
   for a in action_skeleton:
       s = pddl.next_state(s, a)
   print(f"Final state: {s}\n")
   print(f"Is goal satisfied? {pddl.is_goal_satisfied(s)}\n")

   # Find all valid plans.
   print("Planning...")
   for idx_plan, plan in enumerate(bfs):
       print(f"Solution {idx_plan}")
       print("===========")

       # Iterate over all nodes in the plan.
       for node in plan:
           print(node)

See the Python :class:`symbolic.Planner` and
:class:`symbolic.BreadthFirstSearch` references for more details.

