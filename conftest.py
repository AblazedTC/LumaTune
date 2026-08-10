import sys
import os

# Make project root importable when running pytest from the project directory
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
