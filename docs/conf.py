# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.

import os
import sys

sys.path.insert(0, os.path.abspath(".."))

import sphinx_bootstrap_theme


# -- Project information -----------------------------------------------------

project = "symbolic"
copyright = "2020, Toki Migimatsu"
author = "Toki Migimatsu"


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    # 'breathe',  # Generate Sphinx docs from Doxygen
    # 'exhale',  # Run Doxygen and build class/file hierarchy lists
    "sphinx.ext.autodoc",  # Generate docs from docstrings
    "sphinx.ext.autosummary",  # Generate docs from docstrings
    "sphinx.ext.coverage",  # Evaluate documentation coverage
    "sphinx.ext.doctest",  # Test snippets in the docs
    "sphinx.ext.intersphinx",  # Link to external Sphinx docs
    "sphinx.ext.napoleon",  # Parse Google-style docstrings
    "sphinx_autodoc_typehints",  # Use type annotations for parameter types
    "sphinxcontrib.doxylink",  # Link to Doxygen tags
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

# Tell sphinx what the primary language being documented is.
# primary_domain = 'cpp'

# Tell sphinx what the pygments highlight language should be.
# highlight_language = 'cpp'

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "bootstrap"
html_theme_path = sphinx_bootstrap_theme.get_html_theme_path()

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
# html_static_path = ['_static']

# -- Extension configuration -------------------------------------------------

autosummary_generate = True  # Run sphinx-autogen on make
autosummary_imported_members = True  # Document pysymbolic symbols imported in symbolic

# # Setup the breathe extension.
# breathe_projects = {
#     "symbolic": "./_build/doxygen/xml"
# }
# breathe_default_project = "symbolic"

# Setup the doxylink extension.
doxylink = {"symbolic": ("_build/symbolic.tag", "cpp/")}

# # Setup the exhale extension.
# exhale_args = {
#     # These arguments are required
#     "containmentFolder":     "./cpp_api",
#     "rootFileName":          "symbolic.rst",
#     "rootFileTitle":         "C++ API",
#     "doxygenStripFromPath":  "..",
#     # Suggested optional arguments
#     "createTreeView":        True,
#     # TIP: if using the sphinx-bootstrap-theme, you need
#     "treeViewIsBootstrap": True,
#     "exhaleExecutesDoxygen": True,
#     "exhaleDoxygenStdin":    [>
# INPUT = ../include
# ALIASES += "rstref{1}=\\verbatim embed:rst:inline \\1 \\endverbatim"
# ALIASES += "seepython{2}=\\verbatim embed:rst\\n.. seealso:: Python: :func:`\\1.\\2`.\\n\\endverbatim"
# [>,
# }

intersphinx_mapping = {
    "numpy": ("http://docs.scipy.org/doc/numpy/", None),
    "python": ("http://docs.python.org/3/", None),
}
