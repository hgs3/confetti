import pyconfetti
conf = pyconfetti.Confetti("foo bar baz\nabc xyz")
root = conf.get_root()
for dir in root.subdirs:
    for arg in dir.args:
        print(arg.value)
    print("---")
