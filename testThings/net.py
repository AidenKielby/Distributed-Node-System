import math
import random

def sigmoid(x):
    return 1/(1+(math.pow(math.e, -x)))


input_n_1 = 0
input_n_2 = 0

input_to_hidden_weights = [
    [random.randint(1, 60)*0.1, random.randint(1, 60)*0.1],
    [random.randint(1, 60)*0.1, random.randint(1, 60)*0.1]
]

hidden_n_1 = 0
hidden_n_2 = 0

hidden_to_output_weights = [
    [random.randint(1, 60)*0.1, random.randint(1, 60)*0.1]
]

hidden_biases = [0, 0]

output_n = 0

output_bias = 0

def forwardPass(input_n_1, input_n_2, input_to_hidden_weights, hidden_to_output_weights, hidden_biases, output_bias):
    new_hidden_n_1 = sigmoid(
        input_n_1 * input_to_hidden_weights[0][0] + input_n_2 * input_to_hidden_weights[0][1] + hidden_biases[0])
    new_hidden_n_2 = sigmoid(
        input_n_1 * input_to_hidden_weights[1][0] + input_n_2 * input_to_hidden_weights[1][1] + hidden_biases[1])

    new_output_n = sigmoid(
        new_hidden_n_1 * hidden_to_output_weights[0][0] + new_hidden_n_2 * hidden_to_output_weights[0][1] + output_bias)

    return new_hidden_n_1, new_hidden_n_2, new_output_n


def MSE(pred, out):
    return (1/1) * math.pow((out-pred), 2)


def backpropagate(pred, out, input_to_hidden_weights, hidden_to_output_weights, hidden_biases, output_bias, input_n_1, input_n_2, hidden_n_1, hidden_n_2):
    loss = MSE(pred, out)
    outputErrorTerm = (out-pred) * out * (1 - out)

    gradient_w1_out = outputErrorTerm * hidden_n_1
    gradient_w2_out = outputErrorTerm * hidden_n_2
    gradient_b_out = outputErrorTerm

    hidden_contributuion_1 = hidden_to_output_weights[0][0] * outputErrorTerm
    hidden_contributuion_2 = hidden_to_output_weights[0][1] * outputErrorTerm

    hiddenErrorterm1 = hidden_contributuion_1 * hidden_n_1 * (1 - hidden_n_1)
    hiddenErrorterm2 = hidden_contributuion_2 * hidden_n_2 * (1 - hidden_n_2)

    gradient_w1_h1 = hiddenErrorterm1 * input_n_1
    gradient_w2_h1 = hiddenErrorterm1 * input_n_2
    gradient_w1_h2 = hiddenErrorterm2 * input_n_1
    gradient_w2_h2 = hiddenErrorterm2 * input_n_2

    gradient_b_h1 = hiddenErrorterm1
    gradient_b_h2 = hiddenErrorterm2

    return gradient_w1_out, gradient_w2_out, gradient_b_out, gradient_w1_h1, gradient_w2_h1, gradient_w1_h2, gradient_w2_h2, gradient_b_h1, gradient_b_h2


def train_and_run_xor(input1, input2):
    # initialize weights
    input_to_hidden_weights = [[random.uniform(0.1, 6.0), random.uniform(0.1, 6.0)],
                               [random.uniform(0.1, 6.0), random.uniform(0.1, 6.0)]]
    hidden_to_output_weights = [[random.uniform(0.1, 6.0), random.uniform(0.1, 6.0)]]
    hidden_biases = [0, 0]
    output_bias = 0

    # train network
    choices = [[0, 0], [1, 0], [0, 1], [1, 1]]
    results = [0, 1, 1, 0]
    learningRate = 0.01
    for i in range(10000):
        idx = i % 4
        x1, x2 = choices[idx]
        target = results[idx]
        h1 = sigmoid(x1 * input_to_hidden_weights[0][0] + x2 * input_to_hidden_weights[0][1] + hidden_biases[0])
        h2 = sigmoid(x1 * input_to_hidden_weights[1][0] + x2 * input_to_hidden_weights[1][1] + hidden_biases[1])
        out = sigmoid(h1 * hidden_to_output_weights[0][0] + h2 * hidden_to_output_weights[0][1] + output_bias)
        # backpropagate (same as before)
        outputErrorTerm = (target - out) * out * (1 - out)
        grad_w1_out = outputErrorTerm * h1
        grad_w2_out = outputErrorTerm * h2
        grad_b_out = outputErrorTerm
        h_contrib1 = hidden_to_output_weights[0][0] * outputErrorTerm
        h_contrib2 = hidden_to_output_weights[0][1] * outputErrorTerm
        hErr1 = h_contrib1 * h1 * (1 - h1)
        hErr2 = h_contrib2 * h2 * (1 - h2)
        input_to_hidden_weights[0][0] -= learningRate * hErr1 * x1
        input_to_hidden_weights[0][1] -= learningRate * hErr1 * x2
        input_to_hidden_weights[1][0] -= learningRate * hErr2 * x1
        input_to_hidden_weights[1][1] -= learningRate * hErr2 * x2
        hidden_biases[0] -= learningRate * hErr1
        hidden_biases[1] -= learningRate * hErr2
        hidden_to_output_weights[0][0] -= learningRate * grad_w1_out
        hidden_to_output_weights[0][1] -= learningRate * grad_w2_out
        output_bias -= learningRate * grad_b_out

    # run on input
    h1 = sigmoid(input1 * input_to_hidden_weights[0][0] + input2 * input_to_hidden_weights[0][1] + hidden_biases[0])
    h2 = sigmoid(input1 * input_to_hidden_weights[1][0] + input2 * input_to_hidden_weights[1][1] + hidden_biases[1])
    output = sigmoid(h1 * hidden_to_output_weights[0][0] + h2 * hidden_to_output_weights[0][1] + output_bias)
    return output