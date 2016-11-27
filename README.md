# Algorithms to compute the search region

This is the source code for three algorithms to compute the _search region_ in multiobjective optimization. See the refernce paper for more details about the search region.

The project contains three programs:
* classic: an algorithm based on defining points, also knoàwn as "RA" (Redundancy Avoidance)
* classic-filtering: an algorithm based on dominance filtering procedures only, also known as "RE" (Redundancy Elimination)
* neighborhood: an algorithm based on a neighborhood structure between search zones, which is often the most efficient of the three in termes of computation time (details in the reference paper)

## Compiling

You will need `cmake` and a C compiler to build the project.

Then run from the root directory of the project:

    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release ..
    make

This will build the project in the `build` subdirectory.

## Usage

After building, three executables are created, `classic`, `classic-filtering`, and `neighborhood`. Both executables include a short online help (with `-h` or no argument).

Each executable has two operating modes activated with option `-m`:
* with `-m o`, the search region is computed by considering iteratively each point of the instance,
* with `-m s`, the search region is computed by iterating over search zones. At each iteration, it is checkd wether the current search zone contains a point of the instance. In the positive case, the search region is updated while in the negative case, the search zone is isolated. This corresponds to Algorithm 2 in the reference paper.

The input should be a plain text file containing floating point values. Each line starting with a digit is considered as a point and should contain its component values separated by single spaces from the beginning of the line. Other lines are ignored.

## Reference paper

*In case you use this work in your research, please cite the following paper:*

Kerstin Dächert, Kathrin Klamroth, Renaud Lacour, Daniel Vanderpooten, *Efficient computation of the search region in multi-objective optimization*, European Journal of Operational Research, Available online 20 May 2016, http://dx.doi.org/10.1016/j.ejor.2016.05.029.
