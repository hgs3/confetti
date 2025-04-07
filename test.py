import pyconfetti

def print_indent(depth: int) -> None:
    for i in range(depth):
        print("    ", end="")

def print_dir(dir: pyconfetti.Directive, depth: int) -> None:
    print_indent(depth)
    if len(dir.args) > 0:
        for arg in dir.args:
            print("<" + arg.value + ">", end=" ")
    if len(dir) > 0:
        print("[")
        for subdir in dir:
            print_dir(subdir, depth + 1)
        print_indent(depth)
        print("]")
    else:
        print("")

conf = pyconfetti.Confetti("""
# This is a code comment!
foo bar {
    baz {
        123 456
    }
}
abc xyz {
    qux
}
""")

for dir in conf.root:
    print_dir(dir, 0)

for comment in conf.comments:
    print(comment.value)
