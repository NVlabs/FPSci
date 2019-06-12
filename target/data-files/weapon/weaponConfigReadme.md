# Weapon Config Field Descriptions

This file provides information about the weapon to be used in the experiment. Detailed field descriptions are provided below.

* `maxAmmo` refers to the maximum number of clicks a user can make in any given trial
* `firePeriod` controls the minimum fire period allowed in the session (set this to 0 if you want "laser" mode)
* `autoFire` controls whether or not the weapon fires when the left mouse is held, or requires release between fire
* `damagePerSecond` controls the damage done by the weapon per second, when `firePeriod` > 0 the damage per round is set by `damagePerSecond`*`firePeriod`, when `firePeriod` is 0 and `autoFire` is `True` damage is computed based on time hitting the target.
* `fireSound` is the filename/location of the audio clip to use for the weapon firing
* `renderModel` controls whether or not a weapon model is rendered in the first-person view
* `modelSpec` provides an `ArticulatedModel` Any specification for the weapon being used
* `renderBullets` controls whether or not bullets are rendered from the barrel of the weapon when fired
* `renderDecals` controls whether or not bullet hole decals are put on misses when `autoFire` is `False` and `firePeriod` > 0 (i.e. not in laser mode)
* `renderMuzzleFlash` controls whether or not a muzzle flash is rendered for the weapon
 