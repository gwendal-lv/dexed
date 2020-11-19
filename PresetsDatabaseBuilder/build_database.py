# Stores Ddexed/DX7 presets (pre-transform into CSV files) into an .sqlite database

import os
import io
import numpy as np
import pandas as pd
import sqlite3

# This script will explore this folder and its subfolders (recursively)
_presets_parent_folder = "/Volumes/Gwendal_SSD/Datasets/DX7_AllTheWeb"
_output_db_name = "dexed_presets.sqlite"

# Observed in the Jezreel/BRASS-22 and VOICES10 cartridges
_default_preset = np.asarray([1.0, 0.0, 1.0, 0.5, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0])


# Pickled numpy arrays storage in sqlite3 DB
def adapt_array(arr):
    """
    http://stackoverflow.com/a/31312102/190597 (SoulNibbler)
    """
    out = io.BytesIO()
    np.save(out, arr)
    out.seek(0)
    return sqlite3.Binary(out.read())

def convert_array(text):
    out = io.BytesIO(text)
    out.seek(0)
    return np.load(out)

# Converts np.array to TEXT when inserting
sqlite3.register_adapter(np.ndarray, adapt_array)
# Converts TEXT to np.array when selecting
sqlite3.register_converter("NPARRAY", convert_array)

class PresetsDatabase:
    def __init__(self, conn):
        self.conn = conn
        self.cur = conn.cursor()
        self.last_cart_index = -1  # last DX7 cartridge (.csv file) index
        self.last_preset_index = -1
        # TODO create cartridge and preset tables (param table written from __main__)
        self.cur.execute("CREATE TABLE cartridge ("
                         "index_cart INTEGER PRIMARY KEY,"
                         "name TEXT,"
                         "local_path TEXT"
                         ");")
        self.cur.execute("CREATE TABLE preset ("
                         "index_preset INTEGER PRIMARY KEY,"
                         "index_cart INTEGER,"
                         "index_in_cartridge INTEGER,"
                         "name TEXT,"
                         "pickled_params_np_array NPARRAY"
                         ");")

    def insert_from_csv(self, path, local_path, csv_filename):
        cart_df = pd.read_csv(path + "/" + csv_filename, sep=";")
        # add cartridge info, even if it's empty
        self.last_cart_index += 1
        cart_name = csv_filename[0:-4]
        self.cur.execute("INSERT INTO cartridge (index_cart, name, local_path) VALUES (?, ?, ?);",
                         (self.last_cart_index, cart_name, local_path))
        # Preset parameters values (for non-default presets only)
        for i in range(len(cart_df)):
            param_values = np.asarray([float(v) for v in cart_df.iloc[i, 2:].tolist()], dtype=np.float32)
            preset_cart_index = int(cart_df.iloc[i, 0])
            preset_name = cart_df.iloc[i, 1]
            if self._is_preset_empty(param_values, preset_name):
                continue
            self.last_preset_index += 1
            self.cur.execute("INSERT INTO preset (index_preset, index_cart, index_in_cartridge,"
                             "name, pickled_params_np_array) VALUES (?, ?, ?, ?, ?);",
                             (self.last_preset_index, self.last_cart_index, preset_cart_index,
                              preset_name, param_values))

    @staticmethod
    def _is_preset_empty(param_values, preset_name):
        """ Returns whether the given preset (from the cartridge dataframe) is empty or is the default preset. """
        # Is default?
        if np.all(np.isclose(param_values, _default_preset)):
            return True
        # TODO check for default names?
        return False


if __name__ == "__main__":

    # Database creation (from scratch)
    db_filename = _presets_parent_folder + "/" + _output_db_name
    try:
        os.remove(db_filename)
    except OSError:
        pass
    conn = sqlite3.connect(db_filename, detect_types=sqlite3.PARSE_DECLTYPES)
    presets_db = PresetsDatabase(conn)

    # We first read the parameters names
    param_names_df = pd.read_csv(_presets_parent_folder + "/DEXED_PARAMETERS_NAMES.csv", sep=';')
    param_names_df = param_names_df.transpose()
    param_names_df.rename(columns={0: 'name'}, inplace=True)
    # Param indexes starting from zero are added as a separate column
    indexes = np.asarray(range(0, len(param_names_df)), dtype=np.int)
    param_names_df.insert(0, 'index_param', indexes)
    param_names_df.to_sql('param', presets_db.conn, index=False)
    # add column "categorical"? Would better be done later during dataloading...

    # Then we process all CSV files
    for (path, dirs, files) in os.walk(_presets_parent_folder):
        local_path = os.path.relpath(path, _presets_parent_folder)
        #print(local_path)
        #print(dirs)
        #print(files)
        for filename in files:
            if filename.endswith('.csv') and filename != "DEXED_PARAMETERS_NAMES.csv":
                presets_db.insert_from_csv(path, local_path, filename)
        #print('-------')
        presets_db.conn.commit()
    print("CSV files (DX7 cartridges) count = {}".format(presets_db.last_cart_index + 1))

    presets_db.conn.close()
