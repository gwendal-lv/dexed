# Very simple script for searching 1 preset (by name) in the full DB
# Basically intended to check the DB...

import os
import io
import numpy as np
import pandas as pd
import sqlite3

# TODO dirty code organization - to be improved
import build_database  # configures np array sqlite3 adapters

# Name of the preset to be loaded from the DB
_preset_name = 'BRITE RDS '

if __name__ == "__main__":

    db_filename = build_database._presets_parent_folder + "/" + build_database._output_db_name
    conn = sqlite3.connect(db_filename, detect_types=sqlite3.PARSE_DECLTYPES)

    # TODO checker sortie DB
    preset_df = pd.read_sql_query("SELECT * FROM preset WHERE name='{}'".format(_preset_name), conn)
    print(preset_df)
    #preset_values = preset_df.iloc[0, :]['pickled_params_np_array']

    conn.close()
