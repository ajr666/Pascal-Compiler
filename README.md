# Pascal Compiler Project

Welcome to the GitHub repository of our Pascal Compiler Project! This project is at the forefront of developing a robust compiler for the Pascal programming language, transforming Pascal source programs into assembly language. Our goal is to provide a powerful tool that enhances user trust through accurate error detection and a friendly interface.

## Key Features

- **Pascal to Assembly Compilation**: Our compiler is designed to efficiently translate Pascal source programs into assembly language, enabling the execution of Pascal programs on various hardware platforms.

- **Syntax and Semantic Analysis**: We have developed advanced syntax and semantic analysis modules in C++, leveraging the QT framework. This not only ensures the accuracy of compilation but also offers a user-friendly interface.

- **Abstract Syntax Tree Visualization**: With the integration of the QT framework, our compiler visualizes the Abstract Syntax Tree (AST) for each compiled program. This feature significantly enhances user interaction by providing a clear representation of the program structure.

- **Error Compensation Method**: Our innovative approach, named "compensate for errors and continue analyzing correct characters," allows the compiler to detect more compilation errors. This method is particularly effective after implementing two syntax analysis strategies: LL1 and recursive descent.

- **Extensive Grammar Support**: The compiler supports 104 context-free grammars in Pascal, making it highly versatile and capable of compiling a wide range of code segments. It outputs token sequences, ASTs, symbol tables, and identifies nearly 100% of compilation errors.

## Getting Started

To get started with the Pascal Compiler Project, you will need to have C++ and QT installed on your system. Follow these steps:

1. **Clone the repository**:
   ```
   git clone https://github.com/yourrepository/pascal-compiler.git
   ```

2. **Navigate to the project directory**:
   ```
   cd pascal-compiler
   ```

3. **Compile the project** (Ensure QT is properly set up on your system):
   ```
   qmake && make
   ```

4. **Run the compiler**:
   ```
   ./pascalCompiler
   ```

## Contributing

We welcome contributions from the community! Whether you're interested in fixing bugs, adding new features, or improving documentation, your help is greatly appreciated. Please refer to our CONTRIBUTING.md for more information on how to get involved.
