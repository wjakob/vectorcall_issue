from setuptools import Extension, setup

setup(
    ext_modules=[
        Extension(
            name="vectorcall_issue",
            sources=["vectorcall_issue.c"],
        ),
    ]
)
