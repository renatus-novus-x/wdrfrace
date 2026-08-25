# WIRE DRIFT RACERS

## X68000 Wireframe Battle Racing Instruction Manual

```text
                 WIRE DRIFT RACERS

        3 LAPS.  2 CARS.  ONE WINNER.

          TAKE THE LINE.  TAKE THE GATE.
```

Thank you for playing WIRE DRIFT RACERS.
This manual explains how to start the game, control your car, understand the race systems, and fight for victory.

## Contents

1. Story
2. Object of the Game
3. Starting the Game
4. Controls
5. Reading the Race Screen
6. Game Rules
7. Cars and Track Systems
8. CPU LEVEL
9. Basic Technique
10. Boost Strategy
11. Active Gate Strategy
12. Drift Tackle Strategy
13. Catching the Leader
14. Racing the CPU
15. Two-Player Strategy

---

# PART 1: HOW TO PLAY

## 1. Story

The year is 2088.

High above the megacity, a new sport has taken control of the elevated circuits. Its roads and machines are made from lines of light. Its name is WIRE DRIFT.

The rule is simple: two cars enter the ring, and the first car to complete three laps wins.

But active gates feed boost energy into the course. To claim one, a driver may have to abandon the fastest line and dive across a rival's path. When two cars meet, a drift tackle can throw either machine toward the rail. One late decision can decide the whole race.

One red car. One blue car.

In the silent wireframe city, a three-lap battle begins.

## 2. Object of the Game

Complete three laps before your opponent.

- In 1 PLAYER, drive the red P1 car against the blue CPU car.
- In 2 PLAYERS, two drivers compete on one X68000.
- If both cars finish on the same physics update, the result is a DEAD HEAT.
- After the result screen, the game returns to the title in about five seconds.

## 3. Starting the Game

### Title Screen

```text
             1 PLAYER  CPU LEVEL 3
                    2 PLAYERS

                    SPACE START
```

| Action | Keyboard | Gamepad |
|---|---|---|
| Select mode | `W` / `S` or cursor up/down | Up/down on either pad |
| Change CPU LEVEL | `A` / `D` or cursor left/right | Left/right on either pad |
| Confirm | `SPACE` | Button 1 |

CPU LEVEL is used in 1 PLAYER. The default setting is LEVEL 3.

If the title screen is left idle, DEMO REPLAY begins. Press any key or use either gamepad to return to the title.

### HOW TO PLAY Screen

After checking the controls, press `SPACE` or gamepad Button 1 to begin. Press `ESC` to return to the title.

### Start Sequence

After the camera introduction, the race starts in this order:

```text
READY  ->  3  ->  2  ->  1  ->  START
```

The cars cannot move before START. Release the boost button once during the countdown. Holding it before START does not give an early boost.

## 4. Controls

### PLAYER 1

| Function | Keyboard | Gamepad 1 |
|---|---|---|
| Accelerate | `W` | Up |
| Decelerate | `S` | Down |
| Drift inward | `A` | Left |
| Drift outward | `D` | Right |
| Boost | `Q` | Button 1 |
| Brake | `E` | Button 2 |

### PLAYER 2

| Function | Keyboard | Gamepad 2 |
|---|---|---|
| Accelerate | Cursor up | Up |
| Decelerate | Cursor down | Down |
| Drift inward | Cursor left | Left |
| Drift outward | Cursor right | Right |
| Boost | `N` | Button 1 |
| Brake | `M` | Button 2 |

Press `ESC` during a race to return to the title.

## 5. Reading the Race Screen

```text
 P1 LAP 1 OF 3                     CPU LAP 1 OF 3
 BOOST ======..                    BOOST ====....

             ACTIVE GATE LINE

          RED CAR          BLUE CAR

        INNER RAIL        OUTER RAIL
```

| Display | Meaning |
|---|---|
| `P1 LAP` | PLAYER 1 lap count |
| `P2 LAP` | PLAYER 2 lap count |
| `CPU LAP` | CPU lap count in 1 PLAYER |
| `BOOST` | Boost energy in eight steps |
| Bright car | The car is boosting |
| Short line across the course | An active gate lane |

## 6. Game Rules

### 3 LAPS TO WIN

The first car to complete three laps wins. The lap counter increases when the car crosses the start line in the forward direction.

### DEAD HEAT

If both cars reach three laps during the same fixed 20 Hz physics update, the race is a draw. The result screen displays DEAD HEAT.

### RESULT

The result screen shows the winner and both lap counts. Its return timer does not begin until the keys or pad controls held at the finish have been released.

## 7. Cars and Track Systems

### BOOST

Hold the boost button to accelerate beyond the normal maximum speed. The car becomes brighter and the HUD gauge decreases while boost is active.

### ACTIVE GATE

Three gates are placed around the track. Each gate is active in one lane region: inner, center, or outer.

The first correctly aligned car to cross a gate receives boost energy. The gate switches off for about three seconds, then returns in another lane.

### DRIFT TACKLE

Move sideways toward the opponent while making contact to push the other car inward or outward. The attacker also loses some speed, so a careless tackle can hurt both drivers.

A car pushed against a lane limit suffers extra speed loss.

### CATCH-UP BOOST

When the gap becomes large enough, the trailing car slowly recovers boost energy. Speed is not increased automatically. The driver must decide when to use the recovered boost.

## 8. CPU LEVEL

| LEVEL | Behavior | Recommended For |
|---:|---|---|
| 1 | Slow decisions and large gate-target error | First race |
| 2 | Short, cautious boost bursts | Control practice |
| 3 | Standard reaction and boost control | Normal play |
| 4 | Fast gate decisions and longer boosts | Advanced drivers |
| 5 | Fast and accurate decisions | Challenge play |

The CPU avoids boosting when it has a large lead. Every level uses the same car physics, speed limits, gates, and catch-up rules as PLAYER 1.

---

# PART 2: RACING GUIDE

## 9. Basic Technique

### Begin Near the Center

Stay near the center after START. This gives you time to move toward either an inner or outer gate.

### Watch the Next Gate

Do not look only at your opponent. Identify the lane of the next gate early. A late, large drift can cost more time than the gate is worth.

### Use the Brake to Shape the Battle

The brake is not only for stopping. Use it to fall behind a rival, change gate timing, or make an incoming tackle miss.

## 10. Boost Strategy

### Do Not Hold It Forever

Boost is most useful for moving beyond normal maximum speed. Short bursts after a slowdown or gate claim are often more efficient than holding the button continuously.

### Save Energy Before a Gate

A gate approach or drift tackle may reduce your speed. Keeping some energy in reserve lets you accelerate immediately after contact.

### Watch the Gauge When Trailing

The trailing car recovers boost when the gap grows. When the gauge begins to refill, use several short bursts instead of always waiting for a full gauge.

## 11. Active Gate Strategy

### Think One Gate Ahead

Trying to claim every gate causes unnecessary weaving. If the closest gate is too far across the track, prepare for the following gate instead.

### Enter the Rival's Lane

Moving into the opponent's gate lane creates a drift-tackle opportunity. Simple contact without a directed drift slows both cars and may help neither driver.

### Return to the Fast Line

After a gate is claimed, it remains dark for about three seconds. Use that time to return toward the center and prepare for its next lane.

## 12. Drift Tackle Strategy

### Drift Toward the Opponent

Contact alone is not a strong tackle. If the opponent is outside your car, drift outward. If the opponent is inside, drift inward.

### Attack Near the Rail

Pushing a rival into the inner or outer limit causes extra speed loss. This can create enough time to claim the next gate and escape with boost.

### Beware a Double Attack

If both cars drift into each other, both are knocked away and lose substantial speed. A leading driver may be better off yielding the lane.

## 13. Catching the Leader

### Do Not Empty the Gauge

Catch-up recovery means the race is not over after one mistake. Divide recovered energy between closing the gap, fighting for a gate, and accelerating after contact.

### Skip One Gate

Chasing the CPU across every lane can increase the gap. Skip a distant gate, take a shorter line, and fight for the next one after closing in.

### Take the Inside Position

An inside position makes it easier to push the opponent outward. The moment you catch the leader is often your best chance to take control of the race.

## 14. Racing the CPU

### LEVEL 1-2

The CPU has decision delay and gate-target error. Enter the correct lane early and claim gates without taking unnecessary contact risks.

### LEVEL 3

The CPU uses standard boost bursts. Save your own boost until its burst ends, then close the gap while the CPU is resting.

### LEVEL 4-5

Pure speed is not enough. Skip poor gate approaches, use rail-side tackles, and turn catch-up recovery into carefully timed boosts.

## 15. Two-Player Strategy

### Avoid Keyboard Ghosting

The combinations recognized during many simultaneous key presses depend on the keyboard. Use two gamepads when possible.

### Watch Both Gauges

If the rival has little boost, losing one gate may not matter. If the rival is full, slowing the car with a tackle may be worth more than the gate reward.

### Race to the Final Line

The leader defends gates while the trailing car receives recovery and tackle opportunities. A same-update finish is a DEAD HEAT. Keep driving until the final start-line crossing.

---

## QUICK REFERENCE

```text
P1: W/S SPEED   A/D DRIFT   Q BOOST   E BRAKE
P2: UP/DOWN     LEFT/RIGHT  N BOOST   M BRAKE

PAD 1 / PAD 2:
DIRECTION SPEED AND DRIFT   BUTTON 1 BOOST   BUTTON 2 BRAKE

FIRST TO 3 LAPS WINS
```

GOOD LUCK, RACER.
