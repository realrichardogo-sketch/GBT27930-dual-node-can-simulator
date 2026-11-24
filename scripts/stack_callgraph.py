#!/usr/bin/env python3
"""scripts/stack_callgraph.py <ci_dir> <root_function> [indirect_map]

STEP 07-C2.4 static worst-case stack estimate (GCC, NOT ARMCC). Reads every *.ci file produced by
arm-none-eabi-gcc -fcallgraph-info=su (VCG graphs with per-function frame sizes), builds the call
graph and prints the deepest path from <root_function> with the sum of frame sizes.
Indirect calls (function pointers) are not visible to GCC; indirect_map lists, per calling
function, the possible targets ("caller:target1,target2;caller2:target") and is applied as extra
edges. Recursion is reported and cut. Frames marked 'dynamic' are reported."""
import glob
import re
import sys

ci_dir, root = sys.argv[1], sys.argv[2]
extra = sys.argv[3] if len(sys.argv) > 3 else ""
node_re = re.compile(r'node: \{ title: "([^"]+)" label: "([^"]*)"')
edge_re = re.compile(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"')
frames, edges, dyn = {}, {}, set()
for path in glob.glob(ci_dir + "/*.ci"):
    text = open(path, errors="replace").read()
    for title, label in node_re.findall(text):
        name = title.split(":")[-1]
        m = re.search(r"(\d+) bytes \((\w+)", label.replace("\\n", " "))
        if m:
            frames[name] = max(frames.get(name, 0), int(m.group(1)))
            if m.group(2) != "static":
                dyn.add(name)
        else:
            frames.setdefault(name, 0)       # external declaration (no body here)
    for s, t in edge_re.findall(text):
        edges.setdefault(s.split(":")[-1], set()).add(t.split(":")[-1])
for item in filter(None, extra.split(";")):
    caller, targets = item.split(":")
    edges.setdefault(caller, set()).update(filter(None, targets.split(",")))
memo = {}
def depth(fn, stack):
    if fn in stack:
        print("RECURSION cut at", " -> ".join(stack + [fn]))
        return 0, [fn]
    if fn in memo:
        return memo[fn]
    best, best_path = 0, []
    for callee in sorted(edges.get(fn, ())):
        d, p = depth(callee, stack + [fn])
        if d > best:
            best, best_path = d, p
    res = (frames.get(fn, 0) + best, [fn] + best_path)
    memo[fn] = res
    return res
total, path = depth(root, [])
print("root %s: worst-case static stack %d B" % (root, total))
print("path: " + " -> ".join("%s(%d)" % (f, frames.get(f, 0)) for f in path))
missing = sorted(f for f in path if f not in frames or (frames[f] == 0 and f not in edges))
if dyn & set(path):
    print("dynamic frames on path:", sorted(dyn & set(path)))
