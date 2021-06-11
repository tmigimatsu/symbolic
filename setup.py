import setuptools

setuptools.setup(
    name="pysymbolic_v1",
    version="0.1",
    author="Toki Migimatsu",
    author_email="takatoki@cs.stanford.edu",
    description="PDDL symbolic library",
    url="https://github.com/tmigimatsu/symbolic",
    packages=["symbolic_v1"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.6",
)
