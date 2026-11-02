"""Generate master, five CAN lamp boards, standalone slave6 lamp / slave7 key tests and motor profiles."""
from pathlib import Path
import json
from xml.etree import ElementTree as ET
root = Path(__file__).resolve().parents[1]
configure = [{"name": "base", "hidden": True, "generator": "Ninja",
              "binaryDir": "${sourceDir}/build/${presetName}",
              "toolchainFile": "${sourceDir}/cmake/arm-none-eabi.cmake"}]
build = []
run_dir = root / ".run"
run_dir.mkdir(exist_ok=True)
shared = ET.Element("project", version="4")
shared_configs = ET.SubElement(ET.SubElement(shared, "component", name="CMakeSharedSettings"), "configurations")
for board in ["master"] + [f"slave{i}" for i in range(1, 8)] + ["motor"]:
    for kind in ("Debug", "Release"):
        name = f"{board}-{kind}"
        configure.append({"name": name, "inherits": "base", "cacheVariables": {
            "CMAKE_BUILD_TYPE": kind, "ACTIVE_BOARD": board}})
        build.append({"name": name, "configurePreset": name})
        # CLion leaves imported Presets disabled by default. Ship enabled, shared
        # Debug profiles under distinct names so a local preset cannot shadow them.
        profile = f"STM32-{name}"
        ET.SubElement(shared_configs, "configuration", {
            "PROFILE_NAME": profile, "ENABLED": "true" if kind == "Debug" else "false",
            "CONFIG_NAME": kind, "GENERATION_DIR": f"$PROJECT_DIR$/build/{name}",
            "GENERATION_OPTIONS": f"-G Ninja -DACTIVE_BOARD={board}",
            "BUILD_OPTIONS": "-j 8"})
        doc = ET.Element("component", name="ProjectRunConfigurationManager")
        config = ET.SubElement(doc, "configuration", {
            "default": "false", "name": f"Flash {name}", "type": "CMakeRunConfiguration",
            "factoryName": "Application", "REDIRECT_INPUT": "false", "ELEVATE": "false",
            "USE_EXTERNAL_CONSOLE": "false", "EMULATE_TERMINAL": "false", "PASS_PARENT_ENVS_2": "true",
            "PROJECT_NAME": "EnergyControl", "TARGET_NAME": f"EnergyFirmware_{board}_{kind}", "CONFIG_NAME": profile,
            "RUN_PATH": r"C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe",
            "PROGRAM_PARAMS": f'-NoProfile -NonInteractive -ExecutionPolicy Bypass -File "$ProjectFileDir$/tools/flash/flash.ps1" -BuildDir "$CMakeCurrentGenerationDir$" -ExpectedBoard {board} -ExpectedConfiguration {kind}',
            "WORKING_DIR": "$ProjectFileDir$"})
        method = ET.SubElement(config, "method", v="2")
        ET.SubElement(method, "option", {"name": "com.jetbrains.cidr.execution.CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask", "enabled": "true"})
        ET.indent(doc, space="  ")
        ET.ElementTree(doc).write(run_dir / f"Flash-{name}.run.xml", encoding="utf-8", xml_declaration=True)
(root / "CMakePresets.json").write_text(json.dumps({"version": 3, "configurePresets": configure, "buildPresets": build}, indent=2) + "\n", encoding="utf-8")
(root / ".idea").mkdir(exist_ok=True)
ET.indent(shared, space="  ")
ET.ElementTree(shared).write(root / ".idea/cmake.xml", encoding="utf-8", xml_declaration=True)
print(f"Generated {len(build)} presets, {len(build)} shared CLion profiles and {len(build)} board-specific flash targets")
