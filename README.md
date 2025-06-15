# Computer Graphics Studies

## Building

1. Run `./init.sh` to install required dependencies and initialize submodules.
2. Configure the project with CMake using one of the provided presets:

   ```sh
   cmake --preset <preset-name>
   ```

   Presets include `clang-debug`, `clang-release`, `x64-debug`, and `x64-release`.
3. Build using the same preset:

   ```sh
   cmake --build --preset <preset-name>
   ```
