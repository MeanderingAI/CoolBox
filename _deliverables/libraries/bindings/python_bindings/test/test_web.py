import tempfile
import unittest
from pathlib import Path

from ml_toolbox.ml_core import rest_api


class WebHelpersTest(unittest.TestCase):
    def test_form_normalization_and_errors(self):
        form = rest_api.FormSchema(
            '{"fields":{"age":{"type":"integer","required":true,"minimum":18},'
            '"active":{"type":"boolean"}}}'
        )
        request = rest_api.Request(
            rest_api.HttpMethod.POST,
            "/register",
            {"content-type": "application/x-www-form-urlencoded"},
            "age=21&active=on",
        )
        result = form.validate_request(request)
        self.assertTrue(result.valid)
        self.assertEqual(result.values, {"age": 21, "active": True})
        invalid = form.validate('{"age":17}')
        self.assertFalse(invalid.valid)
        self.assertEqual(invalid.errors, {"age": ["minimum"]})
        with self.assertRaises(RuntimeError):
            form.validate("not JSON")

    def test_html_helpers(self):
        with tempfile.TemporaryDirectory() as root:
            Path(root, "page.html").write_text("<h1>{{name}}</h1>", encoding="utf-8")
            response = rest_api.Response.render(root, "page.html", {"name": "<Ada>"})
            self.assertEqual(response.body(), "<h1>&lt;Ada&gt;</h1>")
            self.assertEqual(response.headers()["Content-Type"], "text/html; charset=utf-8")
            self.assertEqual(
                rest_api.Response.html_file(root, "page.html").body(),
                "<h1>{{name}}</h1>",
            )
            with self.assertRaises(ValueError):
                rest_api.Response.render(root, "page.html", {})


if __name__ == "__main__":
    unittest.main()
