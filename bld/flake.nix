{
  description = "way-displays: Auto Manage Your Wayland Displays";

  # nixpkgs and flake-parts are in the flake registry, so we don't have to specify their input URLs
  # https://github.com/NixOS/flake-registry/blob/master/flake-registry.json
  inputs = {
  };

  outputs = inputs@{ nixpkgs, flake-parts, ... }:
    # modularize flake construction https://flake.parts/
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
      ];

      # Explicitly list supported systems. Add more as needed from
      # https://github.com/NixOS/nixpkgs/blob/master/lib/systems/flake-systems.nix
      systems = [ "x86_64-linux" "aarch64-linux" ];
      perSystem = { config, self', inputs', pkgs, system, ... }: {
        # stdenv analyzes the Makefile etc and tries to "do the right thing" without much config
        # https://nixos.org/manual/nixpkgs/unstable/#chap-stdenv
        # Impl: https://github.com/NixOS/nixpkgs/blob/master/pkgs/stdenv/generic/setup.sh
        # Use clangStdenv (1.2GB closure) instead of regular gcc stdenv (322MB closure).
        # packages.default = pkgs.stdenv.mkDerivation {
        packages.default = pkgs.clangStdenv.mkDerivation {
          name = "way-displays";
          version = "1.7.1+dev";
          src = ./.;
          # native means build system (for cross compile)
          nativeBuildInputs = with pkgs; [
            pkg-config
            wayland # native for protocol code gen
            cmocka
          ];

          # built for target system
          buildInputs = with pkgs; [
            wayland # target for libs
            yaml-cpp
            libinput
          ];

          makeFlags = [ "CC=clang CXX=clang++ DESTDIR=$(out) PREFIX= PREFIX_ETC= ROOT_ETC=$(out)/etc" ];
          doCheck = true;
          checkTarget = "test";
          checkInputs = [
            pkgs.cmocka
          ];
        };

        # https://nixos.org/manual/nixpkgs/unstable/#sec-pkgs-mkShell
        devShells.default = pkgs.mkShell {
          packages = with pkgs; [
            ccls
            cppcheck
            include-what-you-use
          ];
          inputsFrom = [ self'.packages.default ];
        };
      };
    };
}
