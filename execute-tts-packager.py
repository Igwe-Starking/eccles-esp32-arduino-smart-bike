# execute-tts-packager.py
# Post-build script for the [env:native] PlatformIO environment.
# Runs after the native TTS packager source is compiled.
# Installs the built exe, runs it if models changed, then uploads LittleFS.

import hashlib
import shutil
import subprocess
from pathlib import Path
from SCons.Script import Import

Import("env")
#disable native upload command
env.Replace(UPLOADCMD="echo [eccles-tts] Native Environment skipping upload")

def directory_hash(folder):
    h = hashlib.sha256()

    for path in sorted(folder.rglob("*")):
        if path.is_file():
            h.update(str(path.relative_to(folder)).encode())

            with open(path, "rb") as f:
                while chunk := f.read(8192):
                    h.update(chunk)

    return h.hexdigest()


def after_build(source, target, env):
    project_dir = Path(env.subst("$PROJECT_DIR"))

    # Models live at eccles/resources/models/
    models_dir = project_dir / "eccles" / "resources" / "models"
    hash_file  = project_dir / ".models_hash"

    if not models_dir.exists():
        print(f"[eccles-tts] ERROR: models directory not found: {models_dir}")
        return

    current_hash  = directory_hash(models_dir)
    previous_hash = hash_file.read_text().strip() if hash_file.exists() else ""

    if current_hash == previous_hash:
        print("[eccles-tts] Models unchanged. Skipping packager.")
        return

    # target[0] is the native build output: .pio/build/native/program.exe
    # Copy and rename it to eccles/tools/bin/eccles-tts-packager.exe.
    # The packager MUST run from eccles/tools/bin/ because when invoked with
    # -platformIO it resolves all paths relative to its own cwd:
    #   ../../eccles/resources/models/   (input wav files)
    #   ../../data/StaticModel.eccles    (output model)
    #   ../../include/StaticModel.h      (output header)
    built_exe = Path(str(target[0]))
    bin_dir   = project_dir / "eccles" / "tools" / "bin"
    packager  = bin_dir / "eccles-tts-packager.exe"

    bin_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(built_exe, packager)
    print(f"[eccles-tts] Installed packager: {packager}")

    print("[eccles-tts] Running packager...")
    result = subprocess.run(
        [str(packager), "-platformIO"],
        cwd=str(bin_dir)
    )

    print(f"[eccles-tts] Packager returned {result.returncode}")

    if result.returncode != 0:
        print("[eccles-tts] Packager failed. Upload cancelled.")
        return

    # Upload LittleFS via subprocess targeting esp32dev explicitly.
    # env.Execute("pio run ...") must NOT be used — it spawns a recursive pio
    # child inside the running build session and exits code 1 with no message.
    # No stdout/stderr redirection so upload output streams live to the terminal.
    print("[eccles-tts] Running LittleFS upload...")
    upload_result = subprocess.run(
        ["pio", "run", "--environment", "esp32dev", "--target", "uploadfs"],
        cwd=str(project_dir)
    )

    if upload_result.returncode == 0:
        # Only save the hash once packager AND upload both succeeded.
        # If upload failed, the hash stays stale so the next build retries
        # the full cycle — packager + upload — instead of silently skipping.
        hash_file.write_text(current_hash)
        print("[eccles-tts] LittleFS upload complete.")
    else:
        print(f"[eccles-tts] LittleFS upload failed (code {upload_result.returncode}).")


env.AddPostAction("$PROGPATH", after_build)
