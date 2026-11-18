"""
Pre-build script: Patch variant header to allow HSE_VALUE override.
The Nucleo variant header hardcodes #define HSE_VALUE 24000000 without
an #ifndef guard, so our -D HSE_VALUE=16000000UL is silently overridden.
This script adds the guard so the command-line value takes precedence.
"""
Import("env")
import os

def patch_hse(source, target, env):
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoststm32")
    # Find the variant header
    variant_dir = os.path.join(framework_dir, "variants", "STM32G4xx")
    for root, dirs, files in os.walk(variant_dir):
        for f in files:
            if f == "variant_NUCLEO_G431RB.h":
                path = os.path.join(root, f)
                with open(path, 'r') as fh:
                    content = fh.read()
                old = "#define HSE_VALUE               24000000"
                new = "#ifndef HSE_VALUE\n#define HSE_VALUE               24000000\n#endif"
                if old in content and "#ifndef HSE_VALUE" not in content:
                    content = content.replace(old, new)
                    with open(path, 'w') as fh:
                        fh.write(content)
                    print("[PATCH] HSE_VALUE guard added to", path)
                else:
                    print("[PATCH] HSE_VALUE already guarded or not found")
                return

env.AddPreAction("buildprog", patch_hse)
