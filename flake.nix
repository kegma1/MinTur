{
  description = "A new Pebble app";

  inputs = {
    pebble.url = "github:pebble-dev/pebble.nix";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    { self, nixpkgs, pebble, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = import nixpkgs {
        inherit system;
      };
    in {
      devShell = pebble.pebbleEnv.${system} {
          packages = with pkgs; [
            typescript-language-server
          ];
      };
    });
}
