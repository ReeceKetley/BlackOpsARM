"""Generate a standalone test from the current production brush functions."""
from pathlib import Path

root = Path(__file__).resolve().parents[2]
source = (root / "src/common/brush_edges.cpp").read_bytes().decode("latin1")
math = (root / "src/universal/com_math.cpp").read_bytes().decode("latin1")


def extract(text, signature):
    start = text.index(signature)
    pos = text.index("{", start)
    depth, end = 1, pos + 1
    while depth:
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


parts = [(root / "tools/port/test_brush_edges.cpp").read_text()]
parts.append(extract(math, "int __cdecl VecNCompareCustomEpsilon("))
for signature in ["bool __cdecl IsPtFormedByThisPlane(", "int __cdecl ThirdPlane(",
                  "const SimplePlaneIntersection **__cdecl NextPointFormedByThisPlane(",
                  "const SimplePlaneIntersection *__cdecl RemoveNextPointFormedByThisPlane(",
                  "char __cdecl FindCycleBFS(", "int __cdecl NumberOfUniquePoints(",
                  "int __cdecl NumberOfOccurancesOfPlane(",
                  "int __cdecl RemovePtsWithPlanesThatOccurLessThanTwice(",
                  "int __cdecl Remove(", "int __cdecl PartitionEdges("]:
    parts.append(extract(source, signature))
output = root / "build/run/test_brush_edges.cpp"
output.write_text("\n\n".join(parts).replace("\r\n", "\n"))
print(output)
