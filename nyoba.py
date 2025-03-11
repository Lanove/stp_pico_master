import numpy as np

# Given resistor values
resistors = np.array([
    95.9, 75.9, 65.1, 80.3, 76.2, 80.8, 83.4, 83.9, 76.2, 84.4,
    71.9, 85.8, 88.8, 59.9, 73.5, 70.6, 71.6, 91.6
])

# Compute the parallel resistance step by step
parallel_resistances = []
inverse_sum = 0

for r in resistors:
    inverse_sum += 1 / r
    parallel_resistances.append(1 / inverse_sum)

print(parallel_resistances)