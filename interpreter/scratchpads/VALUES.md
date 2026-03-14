Here is the complete reconstruction of your interpreter's runtime value architecture, exactly as you designed it:

### The Blueprint of Your Runtime Architecture

**1. Separation of Syntax and State**

* Your Abstract Syntax Tree (AST) strictly handles the code structure and operations (like math or logic).
* Once an AST branch finishes its job, it doesn't hold the result. Instead, it generates a new object allocated on the **heap**.
* This heap object is entirely distinct from the AST; its sole purpose is to represent and manage a runtime value.

**2. The Value Class Hierarchy**

* You have a single **abstract base class** (e.g., `Value` or `RuntimeObject`).
* Beneath it is a family of specialized child classes (e.g., `StringValue`, `BoolValue`, `NumberValue`), all inheriting from the base class.

**3. Compile-Time Type Tagging**

* To identify the underlying type of a base class pointer without slow built-in type checking, you use a dedicated `enum` schema.
* The abstract base class defines a **virtual function** that returns one of these enum constants.
* To make it bulletproof, every child class uses a **`constexpr` variable** (or a specific function) to lock in its specific enum constant. This guarantees a `BoolValue` can only ever identify as a Boolean at compile time.

**4. Safe, Custom Downcasting**

* When you need to "downgrade" (downcast) a generic base pointer to its actual specialized child class, you use a dedicated checking function.
* This function takes the abstract pointer, calls the virtual function to check the `enum` tag, and if it matches the expected type, it outputs the specialized child class pointer safely.