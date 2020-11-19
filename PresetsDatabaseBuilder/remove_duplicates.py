
import os
import io
import numpy as np
import pandas as pd
import sqlite3

# TODO dirty code organization - to be improved
import build_database  # configures np array sqlite3 adapters

if __name__ == "__main__":

    db_filename = build_database._presets_parent_folder + "/" + build_database._output_db_name
    conn = sqlite3.connect(db_filename, detect_types=sqlite3.PARSE_DECLTYPES)

    # TODO we first identify duplicates by name (and we must check whether preset values are actually the same)

    # TODO Then, we go through ALL presets one-by-one, to identify duplicates with different names
    # Might be a quite long computation
