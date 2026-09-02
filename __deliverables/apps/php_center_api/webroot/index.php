<!DOCTYPE html>
<html>
<head><title>CoolBox PHP Demo</title></head>
<body>
<h1>Hello from the PHP interpreter!</h1>
<?php
    $name = isset($_GET['name']) ? $_GET['name'] : 'World';
    echo '<p>Hello, ' . htmlspecialchars($name) . '!</p>';

    $items = ['apples', 'oranges', 'pears'];
    echo '<ul>';
    foreach ($items as $index => $item) {
        echo '<li>' . ($index + 1) . '. ' . $item . '</li>';
    }
    echo '</ul>';

    function fib($n) {
        if ($n < 2) {
            return $n;
        }
        return fib($n - 1) + fib($n - 2);
    }

    echo '<p>fib(10) = ' . fib(10) . '</p>';
?>
</body>
</html>
