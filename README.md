# Pump It Up Keyboard Hook - Now with LTEK and Infinity support

Tool that allows you to use your keyboard or LTEK on Pump It Up.

This fork uses a different approach of hooking the Keyboard inputs, making this compatible with Pump It Up Infinity while keeping compatibility with KPUMP games (tested with Prime 2).  
This specific branch is intended for use with multipump setups, in order to keep consistency between Infinity and Mainline Pump where `Service` and `Test` work in the exact opposite way as they would in Infinity

Shoutouts to all members of KRT - Konmairo Rhythm Team

![KRT - Konmairo Rhythm Team](https://i.imgur.com/d3OlvjU.png)

This fork merges https://github.com/PepinoGz/piu-kb-hook-ltek and https://github.com/Thalesalex/piu-kb-hook to make a ultimate hook supporting both Infinity and the LTEK, all credits to the respective contributors.
# Usage
Download the pre-compiled binary or compile it, then add it to LD_PRELOAD before executing Prime, Prime 2 or Infinity.

# Default keys
Esc to quit game, F1 for test, F2 for service, F3 for coin clear, F5 for player 1 coin, F6 for player 2 coin

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
sudo apt-get install git build-essential libx11-dev gcc-multilib g++-multilib
git clone -b inverted-service-test https://github.com/paranoidnela/piu-kb-hook 
cd piu-kb-hook
make
```

You'll now have a `ltek_kb_hook_inverted.so` file in the aforementioned directory.

# Known Issues

I noticed that K-Pump games (Again, tested with Prime 2) doesn't like when Wine is installed and you have any mouse device conected, making the game segfault just before starting its attract mode, tried fixing this with no success, removing Wine solved the problem. Infinity is not affected by this issue, so I'm not motivated to fix it, if you REALLY need Wine installed, use this fork only with Infinity and just use the original code or other forks for K-Pump

I would like to also note that my LTEK doesn't always get initialised properly causing the pad not to work on some boots of your pump pc, the only solution I found is replugging the pad itself and restarting the game
