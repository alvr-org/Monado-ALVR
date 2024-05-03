# Copyright 2024, Gavin John <gavinnjohn@gmail.com>
# SPDX-License-Identifier: CC0-1.0 OR MIT OR BSL-1.0

{
  inputs = {
    # Whenever an upstream change is merged, update this to
    # the relevant commit and remove the packages from the
    # ...ToUpstream lists below
    nixpkgs.url = "github:NixOS/nixpkgs/e80d1b630036fe33badbc168dfcd071d463b92cf";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};

      devTools = with pkgs; [
        # Tools that are required in order to develop with Monado, but that are not required to build Monado itself
        # These cannot be upstreamed into nixpkgs, as they are not required to build Monado
        # See https://github.com/NixOS/nix/issues/7501 for a discussion on this
        clang
        cmake-format
        git
        gradle
        gradle-completion
      ];

      nativeBuildInputsToUpstream = with pkgs; [
        # If there are any nativeBuildInputs that are not in nixpkgs, add them here
        # nativeBuildInputs are packages that are needed to develop and/or build the project (i.e. tooling)
      ];

      buildInputsToUpstream = with pkgs; [
        # If there are any buildInputs that are not in nixpkgs, add them here
        # buildInputs are any packages that are needed at runtime (i.e. dependencies)
      ];

      package = pkgs.monado.overrideAttrs (oldAttrs: {
        src = ./.;

        nativeBuildInputs = oldAttrs.nativeBuildInputs ++ nativeBuildInputsToUpstream ++ devTools;
        buildInputs = oldAttrs.buildInputs ++ buildInputsToUpstream;

        patches = [];
      });
    in {
      packages.default = package;
      devShells.default = package;
    }
  );
}
