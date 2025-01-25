# Pump It Up Keyboard Hook - Now with Infinity support

Tool that allows you to use your keyboard on Pump It Up.

This fork uses a diffent approach of hooking the Keyboard inputs, making this compatible with Pump It Up Infinity while keeping compatibility with older games (tested with Prime 2).

Shoutouts to all members of KRT - Konmairo Rhythm Team

# Usage
Download the pre-compiled binary or compile it, then add it to LD_PRELOAD before executing Prime, Prime 2 or Infinity.

# Default keys
F1 for test, F2 for service, F5 for player 1 coin, F6 for player 2 coin

Pad layout:
```
Q   E   7   9
  S       5
Z   C   1   3
Pad 1   Pad 2
```

# Compiling

Requires an atleast C++11 compliant compiler.

To compile it yourself, run these commands:

```
sudo apt-get install git build-essential libx11-dev
git clone https://github.com/therathatter/piu-kb-hook
cd piu-kb-hook
make
```

You'll now have a `kb_hook.so` file in the aforementioned directory.
