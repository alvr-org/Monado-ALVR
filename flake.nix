# Copyright 2024, Gavin John <gavinnjohn@gmail.com>
# SPDX-License-Identifier: CC0-1.0 OR MIT OR BSL-1.0

{
  inputs = {
    # Whenever an upstream change is merged, update this to
    # the relevant commit and remove the packages from the
    # ...ToUpstream lists below
    nixpkgs.url = "github:NixOS/nixpkgs/5e8260003c8ec6c5d6ac6064a2ff901677960412";
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
        # Once they are upstreamed to nixpkgs master, remove them from this list
      ];

      buildInputsToUpstream = with pkgs; [
        # If there are any buildInputs that are not in nixpkgs, add them here
        # buildInputs are any packages that are needed at runtime (i.e. dependencies)
        # Once they are upstreamed to nixpkgs master, remove them from this list
        xorg.libXext # Open PR: https://github.com/NixOS/nixpkgs/pull/298083
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
