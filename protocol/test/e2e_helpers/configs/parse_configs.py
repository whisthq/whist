import yaml


def load_yaml_configs(filepath):
    with open(filepath, "r+") as file:
        return yaml.safe_load(file)


def save_configs_to_yaml(filepath, configs):
    with open(filepath, "w+") as file:
        yaml.dump(configs, file, sort_keys=False)


def update_yaml(filepath, config_updates):
    yaml_dict = load_yaml_configs(filepath)
    yaml_dict.update(config_updates)
    save_configs_to_yaml(filepath, yaml_dict)


def is_value_set(configs, key):
    return configs.get(key) is not None
