# Reasons for steering away from a complete C++ project

<aside>
<img src="https://www.notion.so/icons/pen_blue.svg" alt="https://www.notion.so/icons/pen_blue.svg" width="40px" />

***Author***: Shamar Pennant

</aside>

YTBLND’s purpose and design naturally will need a server for inter client communication across devices. Though we recognize that the server could’ve also been coded in C++, we decided to code it in GO instead for the following core reasons

# Five core reasons for choosing GO:

## **1. Go Handles Concurrency Far More Easily**

Go’s concurrency model is purpose‑built for high‑traffic servers, unlike C++ where concurrency is powerful but complex.

**Key advantages:**

- Goroutines are extremely lightweight compared to OS threads.
- Channels provide safe, structured communication between tasks.
- The Go scheduler automatically manages concurrency without manual tuning.
- High‑throughput request handling becomes simpler and more predictable.

## **2. Faster Development and Lower Maintenance Overhead**

Go’s design intentionally removes many of the complexities that slow down C++ development.

**Key advantages:**

- No manual memory management or pointer‑lifetime pitfalls.
- No header files, templates, or complex build chains.
- A small, consistent language surface that’s easy to learn.
- Cleaner codebases that are easier to onboard new developers into.

## **3. Simpler, More Reliable Deployment**

Go produces a single static binary, eliminating many of the deployment challenges common in C++ environments.

**Key advantages:**

- No dependency hell or shared‑library version mismatches.
- No runtime environment or interpreter required.
- Easy to run under systemd with predictable behavior.
- Cross‑compilation is straightforward, enabling flexible CI/CD pipelines.

## **4. Safer and More Stable Long‑Running Processes**

Go’s runtime provides built‑in safety features that reduce operational risk and improve uptime.

**Key advantages:**

- Automatic garbage collection prevents leaks and dangling pointers.
- Bounds checking and race detection catch issues early.
- Fewer undefined‑behavior scenarios compared to C++.
- More stable long‑running services with fewer crash vectors.

## **5. Easier and More Consistent Database Management (MySQL/SQLite)**

Go’s database ecosystem is significantly more ergonomic and safer than C++ equivalents.

**Key advantages:**

- `database/sql` provides a unified interface for MySQL, SQLite, and others.
- Mature drivers offer built‑in connection pooling, prepared statements, and transactions.
- Context‑aware queries simplify timeouts and cancellation.
- Memory‑safe abstractions reduce the risk of leaks or corruption during DB operations.

# Summary

Our decision to transition from a fully C++ codebase to a Go‑based server architecture is rooted in practicality, maintainability, and long‑term operational stability. C++ remains the right choice for the client, where performance, low‑level control, and platform integration matter most. However, server‑side requirements differ significantly, and Go provides clear advantages in that domain.

Go’s lightweight concurrency model, simplified development workflow, and single‑binary deployment approach make it far better suited for modern backend workloads. Its built‑in safety features and runtime guarantees reduce the operational risks associated with long‑running services. Additionally, Go’s unified database interface and mature ecosystem make managing MySQL and SQLite substantially easier and more reliable than equivalent C++ implementations.

By adopting a hybrid architecture—C++ for the client and Go for the server—we leverage the strengths of both languages. This approach improves development velocity, reduces maintenance overhead, enhances system stability, and positions the project for scalable, sustainable growth moving forward.