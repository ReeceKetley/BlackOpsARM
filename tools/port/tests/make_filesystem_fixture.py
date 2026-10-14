"""Small synthetic IWD for native filesystem integration; no retail assets."""
from pathlib import Path
import sys
import zipfile

directory = Path(sys.argv[1])
directory.mkdir(parents=True, exist_ok=True)
with zipfile.ZipFile(directory / "Fixture.IWD", "w") as archive:
    archive.comment = b"native original-reader integration"
    archive.writestr("Data/Pattern.bin", bytes(i % 251 for i in range(70000)),
                     compress_type=zipfile.ZIP_DEFLATED)
    archive.writestr("Data/Stored.txt", b"stored",
                     compress_type=zipfile.ZIP_STORED)
