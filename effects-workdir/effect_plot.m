quad_test = csvread("quad_wave.csv");
tri_test = csvread("triwave.csv");
easing_test = csvread("easing.csv");

hold on; grid on;
plot(quad_test(:, 1), quad_test(:, 2), 'b');
plot(tri_test(:, 1), tri_test(:, 2), 'g');
plot(easing_test(:, 1), easing_test(:, 2), 'r');
