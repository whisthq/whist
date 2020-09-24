from bs4 import BeautifulSoup
import requests
import json


class ProductHunt:
    def parse_html(self, url):
        """returns the BeautifulSoup Text of
        ProductHunt
        """
        html_doc = requests.get(url)
        return BeautifulSoup(html_doc.text)

    def find_makers_and_hunters(self):
        """returns the makers and hunters of today's producthunt"""
        b = self.parse_html("http://www.producthunt.com/")
        l = b.find_all("a", class_="link_523b9")

        links = [("https://www.producthunt.com" + x["href"]) for x in l]
        makers, hunters = [], []

        for product in links:
            _soup = self.parse_html(product)
            _people = [
                x.text
                for x in _soup.find_all(
                    "div",
                    class_="font_9d927 small_231df semiBold_e201b title_2bd7a lineHeight_042f1 underline_57d3c",
                )
            ]

            if _people:
                hunter = _people.pop(0)
                hunters.append(hunter)
            makers.append(_people)

        flatten_makers = sum(makers, [])

        return flatten_makers, hunters