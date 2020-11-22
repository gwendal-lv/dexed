
import os
import io
import numpy as np
import pandas as pd
import sqlite3

# TODO dirty code organization - to be improved
import build_database  # configures np array sqlite3 adapters


class DuplicatesRemover:
    def __init__(self, db_filename=None, log_filename=None):
        if db_filename is None:
            self.db_filename = build_database._presets_parent_folder + "/" + build_database._output_db_name
        else:
            self.db_filename = db_filename
        self.conn = sqlite3.connect(self.db_filename, detect_types=sqlite3.PARSE_DECLTYPES)
        self.cur = self.conn.cursor()
        
        self.log_filename = log_filename

        # Some sub-folders might contain a lot of duplicates, thus they are "low-priority" to try to keep a clean
        # structure. In case of duplicate, the ones in this folder will be discarded.
        self.low_priority_subfolders = ["Dexed v1_0 (Black Winny)", "LiVeMuSiC", "Bridge Music Recording Studio"]

    def get_preset_indexes(self):
        preset_indexes = pd.read_sql_query("SELECT index_preset FROM preset", self.conn)
        return preset_indexes['index_preset'].to_numpy(dtype=np.int)

    def is_low_priority_cart(self, index_cart):
        cart_df = pd.read_sql_query("SELECT * FROM cartridge WHERE index_cart={}".format(index_cart), self.conn)
        cart_local_path = cart_df['local_path'].values[0]
        is_low_priority = False
        for subfolder in self.low_priority_subfolders:
            if subfolder in cart_local_path:
                is_low_priority = True
                break
        return is_low_priority
    
    def _log(self, log_str):
        print(log_str)
        if self.log_filename is not None:
            with open(self.log_filename, "a") as log_file:
                log_file.write(log_str + '\n')

    def remove_by_name(self):
        """ identify and remove duplicates by name (check whether preset values are actually the same) """
        nb_removed_presets = 0
        preset_indexes = self.get_preset_indexes()
        for i in range(len(preset_indexes)):
            index_preset = preset_indexes[i]
            # Preset name to be searched - we also retrieve cart info.
            # TODO replace Dirty SQL queries by parametrized queries
            preset_df = pd.read_sql_query("SELECT * FROM preset WHERE index_preset={}".format(index_preset), self.conn)
            if len(preset_df) == 0:
                continue  # The index might have already been removed.
            preset_name = preset_df['name'].values[0]
            index_cart = preset_df['index_cart'].values[0]
            if self.is_low_priority_cart(index_cart):
                continue  # If the folder is low-priority, we just skip to the new preset
            preset_values = preset_df['pickled_params_np_array'].values[0]
            # we select all presets having the same name
            similar_presets_df = pd.read_sql_query("SELECT * FROM preset WHERE name={}".format(repr(preset_name)), self.conn)
            for j in range(len(similar_presets_df)):
                similar_preset = similar_presets_df.iloc[j]
                if similar_preset['index_preset'] == index_preset:
                    continue  # We do not remove the ref preset iself!
                # But we remove the similar preset from the DB if they are actually the same
                if np.allclose(preset_values, similar_preset['pickled_params_np_array']):
                    self.cur.execute("DELETE FROM preset WHERE index_preset={}".format(similar_preset['index_preset']))
                    nb_removed_presets += 1
            if i % 100 == 0:
                self._log("(step {}/{}: presets removed by name: {})".format(i, len(preset_indexes), nb_removed_presets))
            self.conn.commit()
        self._log("======= Search by name: {} duplicates removed ========".format(nb_removed_presets))

    def remove_by_value(self):
        """ Runs through all presets, and searches all other presets to identify duplicates by parameter values. """
        nb_removed_presets = 0
        all_presets_df = pd.read_sql_query("SELECT * FROM preset", self.conn)
        preset_indexes = all_presets_df['index_preset'].to_numpy(dtype=np.int)
        is_preset_removed_from_db = [False for _ in range(preset_indexes.shape[0])]  # index: i or j (not index_preset)
        for i in range(len(preset_indexes) - 1):
            if not is_preset_removed_from_db[i]:
                index_preset = preset_indexes[i]
                preset_values = all_presets_df.iloc[i]['pickled_params_np_array']
                # No "low-priority" check
                # We test all remaining presets
                other_preset_names = []
                for j in range(i+1, len(preset_indexes)):
                    if not is_preset_removed_from_db[j]:
                        index_other_preset = preset_indexes[j]
                        other_preset_values = all_presets_df.iloc[j]['pickled_params_np_array']
                        if np.allclose(preset_values, other_preset_values, atol=0.5/128.0):  # Very similar preset ?
                            preset_name = all_presets_df.iloc[i]['name']
                            other_preset_names.append(all_presets_df.iloc[j]['name'])
                            self.cur.execute("DELETE FROM preset WHERE index_preset={}".format(index_other_preset))
                            is_preset_removed_from_db[j] = True
                            nb_removed_presets += 1
                # TODO save name of deleted presets
                if len(other_preset_names) > 0 and False:  # deactivated
                    other_names = ""
                    for k in range(len(other_preset_names)):
                        other_names += other_preset_names[k] + '\n'
                    #self.cur.execute("UPDATE preset SET other_names=? WHERE index_preset=?;", (other_names, index_preset))
            if i % 100 == 0:
                self._log("(step {}/{}: presets removed by value: {})".format(i, len(preset_indexes), nb_removed_presets))
            self.conn.commit()
        self._log("======= Search by value: {} duplicates removed ========".format(nb_removed_presets))


if __name__ == "__main__":

    duplicates_remover = DuplicatesRemover()

    # Removal by name (identical name and values necessary for removal)
    #duplicates_remover.remove_by_name()  # Uncomment to process

    # Addition of a new column to save the name of duplicates - call only once
    #cur.execute("ALTER TABLE preset ADD other_names TEXT;")

    # TODO Then, we go through ALL presets one-by-one, to identify duplicates (by preset values) with different names
    # Might be a quite long computation
    duplicates_remover.remove_by_value()

    # TODO Finally, we can remove cartridges which contain no preset

    # Possible vacuum - uncomment
    #cur.execute("VACUUM;")

    duplicates_remover.conn.close()
