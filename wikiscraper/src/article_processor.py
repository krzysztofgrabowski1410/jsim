import collections
import os
import re

import requests
import pandas as pd
from typing import Optional, Dict, List

from bs4 import BeautifulSoup


class ArticleNotFound(Exception):
    """Raised when the wiki article is not found (404)."""
    pass


class ArticleProcessor:
    """
    Handles scraping and parsing of Wiki articles.
    Decouples logic from I/O by accepting HTML content or fetching it via a separate method.
    """

    def __init__(self, base_url: str = "https://bulbapedia.bulbagarden.net/wiki/", offline_path: Optional[str] = None):
        """
        Args:
            base_url: The base URL for the wiki.
            offline_path: Path to a directory containing HTML files to read instead of fetching from the web.
                          Useful for testing and offline development.
        """
        self.base_url = base_url
        self.offline_path = offline_path
        self.session = requests.Session()
        # User agent to avoid being blocked
        self.session.headers.update({
            "User-Agent": "WikiScraperBot/1.0 (Educational Project)"
        })

    def fetch_article(self, search_term: str) -> str:
        """
        Fetches the HTML content for a given search term.
        If offline_path is set, tries to read from a local file named {search_term}.html
        """
        formatted_term = search_term.replace(" ", "_")

        if self.offline_path:
            file_path = os.path.join(self.offline_path, f"{formatted_term}.html")
            try:
                if not os.path.exists(file_path):
                    raise FileNotFoundError

                with open(file_path, 'r', encoding='utf-8') as f:
                    return f.read()
            except (FileNotFoundError, OSError):
                # Mimic 404 behavior for offline mode
                raise ArticleNotFound(f"Local article '{search_term}' not found at {file_path}")

        url = f"{self.base_url}{formatted_term}"

        try:
            response = self.session.get(url)
            response.raise_for_status()
            return response.text
        except requests.exceptions.HTTPError as e:
            if e.response.status_code == 404:
                raise ArticleNotFound(f"Article '{search_term}' not found at {url}")
            raise ConnectionError(f"HTTP error fetching '{search_term}': {e}")
        except requests.exceptions.RequestException as e:
            raise ConnectionError(f"Failed to fetch article for '{search_term}': {e}")

    def get_summary(self, html_content: str) -> str:
        """
        Retrieves the first paragraph of the wiki page.
        Skips static elements like menus, tables (infoboxes), and TOC.
        """
        soup = BeautifulSoup(html_content, 'html.parser')

        content_div = soup.find('div', class_='mw-parser-output')
        if not content_div:
            content_div = soup.find('div', id='mw-content-text')

        if not content_div:
            return "Could not find content area."

        paragraphs = content_div.find_all('p')

        for p in paragraphs:
            if p.find_parent('table'):
                continue

            if p.find_parent('div', class_='toc') or p.find_parent('div', id='toc'):
                continue

            if p.find_parent('div', class_='thumb'):
                continue

            text = p.get_text().strip()

            if len(text) > 1:
                return text

        return "No summary paragraph found."

    def extract_table(self, html_content: str, table_number: int, first_row_is_header: bool = False) -> pd.DataFrame:
        """
        Extracts the n-th table from the page.
        Uses pandas read_html for robust parsing (rowspan/colspan).
        Assumes first column is row headers (index_col=0).
        """
        soup = BeautifulSoup(html_content, 'html.parser')
        tables = soup.find_all('table')

        if table_number < 1 or table_number > len(tables):
            raise ValueError(f"Table number {table_number} out of range. Found {len(tables)} tables.")

        target_table = tables[table_number - 1]

        try:
            from io import StringIO
            header_arg = 0 if first_row_is_header else None

            dfs = pd.read_html(StringIO(str(target_table)), header=header_arg, index_col=0)

            if not dfs:
                return pd.DataFrame()
            return dfs[0]
        except Exception as e:
            raise ValueError(f"Failed to parse table with pandas: {e}")

    def count_words(self, html_content: str) -> Dict[str, int]:
        """
                Counts words in an article, excluding static page elements.
                Focuses on the main content area.
                """
        soup = BeautifulSoup(html_content, 'html.parser')

        # Focus on content area to exclude menus, sidebars, footers
        content_div = soup.find('div', class_='mw-parser-output')
        if not content_div:
            content_div = soup.find('div', id='mw-content-text')

        if not content_div:
            return {}

        # Remove scripts, styles from the content area just in case
        for script in content_div(["script", "style", "meta", "noscript"]):
            script.decompose()

        # Get text only from the content
        text = content_div.get_text()

        # Simple tokenization
        # using \w+ might include numbers, which is usually acceptable for "words" in this context
        # or we could use [a-zA-Z]+ but \w is standard for "word characters"
        words = re.findall(r'\b\w+\b', text.lower())

        return dict(collections.Counter(words))

    def get_links(self, html_content: str) -> List[str]:
        """
        Extracts internal links from the main content.
        """
        soup = BeautifulSoup(html_content, 'html.parser')
        content_div = soup.find('div', class_='mw-parser-output') or soup.find('div', id='mw-content-text')

        links = []
        if content_div:
            for a in content_div.find_all('a', href=True):
                href = a['href']
                # Filter for internal wiki links, excluding special pages
                if href.startswith('/wiki/') and ':' not in href:
                    clean_term = href.replace('/wiki/', '')
                    links.append(clean_term)

        return links
