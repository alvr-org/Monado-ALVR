#!/usr/bin/env python3
# Copyright 2020-2025, Collabora, Ltd.
# Copyright 2024-2026, NVIDIA CORPORATION.
# SPDX-License-Identifier: BSL-1.0
"""Parse a JSON file describing interaction profiles and bindings."""

import json
import copy
from operator import attrgetter

def wl(f, *args, endNewLine=True):
    """Write lines"""
    s = '\n'.join(args)
    if endNewLine:
        s += '\n'
    f.write(s)

def find_component_in_list_by_name(name, component_list, subaction_path=None, identifier_json_path=None):
    """Find a component with the given name in a list of components."""
    for component in component_list:
        if component.component_name == name:
            if subaction_path is not None and component.subaction_path != subaction_path:
                continue
            if identifier_json_path is not None and component.identifier_json_path != identifier_json_path:
                continue
            return component
    return None


def steamvr_subpath_name(steamvr_path, subpath_type):
    if subpath_type == "pose":
        return steamvr_path.replace("/input/", "/pose/")

    if subpath_type == "trigger" or subpath_type == "button":
        return steamvr_path.replace("squeeze", "grip")

    if subpath_type == "joystick":
        return steamvr_path.replace("thumbstick", "joystick")

    return steamvr_path


class PathsByLengthCollector:
    """Helper class to sort paths by length, useful for creating fast path
    validation functions.
    """

    def __init__(self):
        self.by_length = dict()

    def add_path(self, path):
        length = len(path)
        if length in self.by_length:
            self.by_length[length].add(path)
        else:
            self.by_length[length] = {path}

    def add_paths(self, paths):
        for path in paths:
            self.add_path(path)

    def to_dict_of_lists(self):
        ret = dict()
        for length, set_per_length in self.by_length.items():
            ret[length] = list(set_per_length)
        return ret


def dpad_paths(identifier_path, center):
    paths = [
        identifier_path + "/dpad_up",
        identifier_path + "/dpad_down",
        identifier_path + "/dpad_left",
        identifier_path + "/dpad_right",
    ]

    if center:
        paths.append(identifier_path + "/dpad_center")

    return paths


class DPad:
    """Class holding per identifier information for dpad emulation."""

    @classmethod
    def parse_dpad(dpad_cls,
                   identifier_path,
                   component_list,
                   dpad_json):
        center = dpad_json["center"]
        position_str = dpad_json["position"]
        activate_str = dpad_json.get("activate")

        position_component = find_component_in_list_by_name(position_str,
                                                            component_list)
        activate_component = find_component_in_list_by_name(activate_str,
                                                            component_list)

        paths = dpad_paths(identifier_path, center)

        return DPad(center,
                    paths,
                    position_component,
                    activate_component)

    def __init__(self,
                 center,
                 paths,
                 position_component,
                 activate_component):
        self.center = center
        self.paths = paths
        self.position_component = position_component
        self.activate_component = activate_component


def resolve_monado_binding(profile_name, identifier_json_path, json_subpath, component_name):
    """Look up the Monado enum for one component.

    ``false`` means the OpenXR path is exposed with no Monado input or output.
    A missing key is an error: the component name and the monado_bindings key
    have to be the same string.
    """
    if "monado_bindings" not in json_subpath:
        raise RuntimeError(
            f"Interaction profile {profile_name} path {identifier_json_path} "
            f"has no monado_bindings object.")

    monado_bindings = json_subpath["monado_bindings"]
    if component_name not in monado_bindings:
        raise RuntimeError(
            f"Interaction profile {profile_name} path {identifier_json_path} "
            f"component '{component_name}' is missing from monado_bindings. "
            f"Use an XRT_INPUT or XRT_OUTPUT name, or false for no Monado mapping.")

    monado_binding = monado_bindings[component_name]
    # JSON false is the explicit "expose the path, bind no device input" case.
    if monado_binding is False:
        return None
    if isinstance(monado_binding, str):
        return monado_binding

    raise RuntimeError(
        f"Interaction profile {profile_name} path {identifier_json_path} "
        f"component '{component_name}' has monado_bindings value {monado_binding!r}. "
        f"Use an XRT_INPUT or XRT_OUTPUT name, or false for no Monado mapping.")


class Component:
    """Components correspond with the standard OpenXR components click, touch,
    force, value, x, y, twist, pose
    """

    @classmethod
    def parse_components(component_cls,
                         profile_name,
                         subaction_path,
                         identifier_json_path,
                         json_subpath):
        """
        Turn a Identifier's component paths into a list of Component objects.
        """

        component_list = []
        for component_name in json_subpath["components"]:  # click, touch, ...
            matched_dpad_emulation = None
            if ("dpad_emulation" in json_subpath and
                    json_subpath["dpad_emulation"]["position"] == component_name):
                matched_dpad_emulation = json_subpath["dpad_emulation"]

            monado_binding = resolve_monado_binding(profile_name,
                                                    identifier_json_path,
                                                    json_subpath,
                                                    component_name)

            steamvr_path = steamvr_subpath_name(identifier_json_path, json_subpath["type"])
            if "steamvr_path" in json_subpath:
                steamvr_path = json_subpath["steamvr_path"]

            c = Component(subaction_path,
                          identifier_json_path,
                          steamvr_path,
                          json_subpath["localized_name"],
                          json_subpath["type"],
                          component_name,
                          matched_dpad_emulation,
                          monado_binding,
                          json_subpath["components"])
            component_list.append(c)

        return component_list

    def __init__(self,
                 subaction_path,
                 identifier_json_path,
                 steamvr_path,
                 subpath_localized_name,
                 subpath_type,
                 component_name,
                 dpad_emulation,
                 monado_binding,
                 components_for_subpath):
        self.subaction_path = subaction_path
        self.identifier_json_path = identifier_json_path  # note: starts with a slash
        self.steamvr_path = steamvr_path
        self.subpath_localized_name = subpath_localized_name
        self.subpath_type = subpath_type
        self.component_name = component_name
        self.dpad_emulation = dpad_emulation
        self.monado_binding = monado_binding

        # click, touch etc. components under the subpath of this component.
        # Only needed for steamvr profile gen.
        self.components_for_subpath = components_for_subpath

    def get_full_openxr_paths(self):
        """Concrete OpenXR component paths for this component."""
        basepath = self.subaction_path + self.identifier_json_path

        if self.component_name == "position":
            return [
                basepath + "/x",
                basepath + "/y",
            ]

        if self.component_name == "haptic":
            return [basepath]

        return [basepath + "/" + self.component_name]

    def get_full_path(self):
        return self.subaction_path + self.identifier_json_path + '/' + self.component_name

    def is_input(self):
        # only haptics is output so far, everything else is input
        return self.component_name != "haptic"

    def has_dpad_emulation(self):
        return self.dpad_emulation is not None

    def is_output(self):
        return not self.is_input()


class Identifier:
    """A Identifier is a OpenXR identifier with a user path, such as button
    X, a trackpad, a pose such as aim. It can have one or more features, even
    tho outputs doesn't include a component/feature path a output identifier
    will have a haptic output feature.
    """

    @classmethod
    def parse_identifiers(indentifer_cls, profile_name, json_profile):
        """Turn a profile's input paths into a list of Component objects."""

        json_subaction_paths = json_profile["subaction_paths"]
        json_subpaths = json_profile["subpaths"]

        identifier_list = []
        for subaction_path in json_subaction_paths:  # /user/hand/*
            for json_path in sorted(json_subpaths.keys()):  # /input/*, /output/*
                # json object associated with a subpath (type, localized_name, ...)
                json_subpath = json_subpaths[json_path]

                # Oculus Touch a,b/x,y components only exist on one controller
                if "side" in json_subpath and "/user/hand/" + json_subpath["side"] != subaction_path:
                    continue

                # Full path to the identifier
                identifier_path = subaction_path + json_path

                component_list = Component.parse_components(profile_name,
                                                            subaction_path,
                                                            json_path,
                                                            json_subpath)

                dpad = None
                if "dpad_emulation" in json_subpath:
                    dpad = DPad.parse_dpad(identifier_path,
                                           component_list,
                                           json_subpath["dpad_emulation"])

                i = Identifier(subaction_path,
                              identifier_path,
                              json_path,
                              component_list,
                              dpad)
                identifier_list.append(i)

        return identifier_list

    def __init__(self,
                 subaction_path,
                 identifier_path,
                 json_path,
                 component_list,
                 dpad):
        self.subaction_path = subaction_path
        self.identifier_path = identifier_path
        self.json_path = json_path
        self.components = component_list
        self.dpad = dpad
        return


def is_valid_version(version):
    """Returns whether the version is a valid version that is > 0.0"""
    if version is None:
        return False
    # 0.0 is a placeholder for "no particular version"
    if version["major"] == '0' and version["minor"] == '0':
        return False
    return True


class FeatureSet:
    """An AND of requirements (versions and/or extensions) under which a binding becomes available"""

    def __init__(self, required_extensions=None, required_version=None):
        self.required_extensions = frozenset(required_extensions if required_extensions is not None else [])
        self.required_version = required_version if required_version is not None else {"major": "0", "minor": "0"}

    def as_tuple(self):
        return (self.required_version["major"],
                self.required_version["minor"],
                sorted(self.required_extensions))

    def __str__(self) -> str:
        return f"{self.as_tuple()}"

    def is_more_restrictive_than(self, other):
        if other.required_extensions.issuperset(self.required_extensions):
            # other requires extensions we don't
            return False
        if is_valid_version(other.required_version) and not is_valid_version(self.required_version):
            # other bounds version, we don't
            return False
        if is_valid_version(other.required_version) and is_valid_version(self.required_version):
            if other.required_version["major"] != self.required_version["major"]:
                # different major versions - not fully implemented, but this seems right
                return False
            if int(other.required_version["minor"]) > int(self.required_version["minor"]):
                # other has a higher lower bound on minor version than us
                return False

        return True

    def and_also(self, other):
        result = copy.deepcopy(self)
        result.required_extensions = self.required_extensions | other.required_extensions
        if not is_valid_version(result.required_version):
            result.required_version = other.required_version
        elif is_valid_version(other.required_version):
            if result.required_version["major"] != other.required_version["major"]:
                raise NotImplementedError("Major version mismatch not handled")
            if int(result.required_version["minor"]) < int(other.required_version["minor"]):
                result.required_version["minor"] = other.required_version["minor"]

        return result


class Availability:
    """An OR of FeatureSets, where any one of them being satisfied means a binding becomes available"""

    def __init__(self, feature_sets, optimize=True):
        if not optimize:
            self.feature_sets = set(feature_sets)
            return

        self.feature_sets = set()
        for feature_set in feature_sets:
            self.add_in_place(feature_set)

    def __str__(self) -> str:
        return f"{[str(fs) for fs in sorted(self.feature_sets, key=FeatureSet.as_tuple)]}"

    """Add an additional way for this availability to be satisfied"""
    def add_in_place(self, new_feature_set):
        for existing_feature in list(self.feature_sets):
            if existing_feature.is_more_restrictive_than(new_feature_set):
                self.feature_sets.remove(existing_feature)
            elif new_feature_set.is_more_restrictive_than(existing_feature):
                return
        self.feature_sets.add(new_feature_set)

    """Add an additional restriction to all feature sets"""
    def intersect_with_feature(self, feature_set):
        result = Availability(feature_sets=[])
        for existing_feature in self.feature_sets:
            result.add_in_place(existing_feature.and_also(feature_set))
        return result

    """Combine two availabilities into one that is satisfied if either is"""
    def union(self, other):
        result = copy.deepcopy(self)
        for feature_set in other.feature_sets:
            result.add_in_place(feature_set)

    """Combine two availabilities into one that is satisfied if both are.
       Note that this acts as a cartesian product followed by an at least n^2 op on that.
    """
    def intersection(self, other):
        features = []
        for feature_set in other.feature_sets:
            inter = self.intersect_with_feature(feature_set)
            features.extend(inter.feature_sets)

        return Availability(features)


class Profile:
    """An interactive bindings profile."""

    def __init__(self, profile_name, json_profile):
        """Construct an profile."""
        self.parent_profiles = set()
        self.name = profile_name
        self.localized_name = json_profile['title']
        self.profile_type = json_profile["type"]
        self.monado_device_enum = json_profile["monado_device"]
        self.validation_func_name = Profile.__strip_profile_prefix(
            profile_name).replace("/", "_")
        self.extension_name = json_profile.get("extension")
        self.extended_by = json_profile.get("extended_by")
        if self.extended_by is None:
            self.extended_by = []

        ov = json_profile.get("openxr_version")
        if ov is None:
            self.openxr_version_promoted = { "major" : "0", "minor" : "0" }
        else:
            promoted = ov.get("promoted")
            if promoted is None:
                self.openxr_version_promoted = { "major" : "0", "minor" : "0" }
            else:
                self.openxr_version_promoted = { "major" : promoted.get("major"), "minor" : promoted.get("minor") }

        self.is_virtual = profile_name.startswith("/virtual_profiles/")
        self.identifiers = Identifier.parse_identifiers(profile_name, json_profile)

        self.steamvr_controller_type = None
        if "steamvr_controllertype" in json_profile:
            self.steamvr_controller_type = json_profile["steamvr_controllertype"]

        self.__update_component_list()
        collector = PathsByLengthCollector()
        for component in self.components:
            collector.add_paths(component.get_full_openxr_paths())
        for identifier in self.identifiers:
            if any(component.component_name not in ["pose", "haptic"] for component in identifier.components):
                collector.add_path(identifier.identifier_path)
        self.subpaths_by_length = collector.to_dict_of_lists()

        collector = PathsByLengthCollector()
        for identifier in self.identifiers:
            if not identifier.dpad:
                continue
            collector.add_path(identifier.identifier_path)
        self.dpad_emulators_by_length = collector.to_dict_of_lists()

        collector = PathsByLengthCollector()
        for identifier in self.identifiers:
            if not identifier.dpad:
                continue
            path = identifier.identifier_path
            collector.add_paths(identifier.dpad.paths)
        self.dpad_paths_by_length = collector.to_dict_of_lists()

    @classmethod
    def __strip_profile_prefix(cls, profile_path):
        return profile_path.replace("/interaction_profiles/", "").replace("/virtual_profiles/", "")

    def is_parent_profile(self, child_profile):
        if child_profile == self:
            return False
        if child_profile.extended_by is None:
            return False
        parent_path = Profile.__strip_profile_prefix(self.name)
        return parent_path in child_profile.extended_by

    def merge_parent_profiles(self):
        self.identifiers = self.__get_merged_identifiers_helper({}).values()
        self.__update_component_list()

    def __get_merged_identifiers_helper(self, identifier_map):
        for ident in self.identifiers:
            if ident.identifier_path not in identifier_map:
                identifier_map[ident.identifier_path] = copy.deepcopy(ident)
                continue
            child_indent = identifier_map[ident.identifier_path]
            if child_indent.dpad is None:
                child_indent.dpad = ident.dpad
            child_comps = child_indent.components
            for parent_comp in ident.components:
                parent_path = parent_comp.get_full_path()
                child_exists = False
                for child_comp in child_comps:
                    if child_comp.get_full_path() == parent_path:
                        child_exists = True
                        break
                if not child_exists:
                    child_comps.append(parent_comp)

        parent_profiles = self.parent_profiles
        if parent_profiles is None or len(parent_profiles) == 0:
            return identifier_map
        else:
            for parent in parent_profiles:
                parent.__get_merged_identifiers_helper(identifier_map)
            return identifier_map

    def __update_component_list(self):
        self.components = []
        for identifier in self.identifiers:
            self.components += identifier.components
        self.components = sorted(self.components, key=attrgetter("steamvr_path"))

    def availability(self):
        result = Availability(feature_sets=[])
        has_requirements = False
        if is_valid_version(self.openxr_version_promoted):
            result.add_in_place(FeatureSet(required_version=self.openxr_version_promoted))
            has_requirements = True
        if self.extension_name is not None:
            result.add_in_place(FeatureSet(required_extensions=[self.extension_name]))
            has_requirements = True
        if not has_requirements:
            result.add_in_place(FeatureSet())

        return result


class Bindings:
    """A collection of interaction profiles used in bindings."""

    @classmethod
    def parse(cls, json_root):
        """Parse an entire bindings.json into a collection of Profile objects.
        """
        return cls(json_root)

    @classmethod
    def load_and_parse(cls, file):
        """Load a JSON file and parse it into Profile objects."""
        with open(file) as infile:
            json_root = json.loads(infile.read())
            return cls.parse(json_root)

    def __init__(self, json_root):
        """Construct a bindings from a dictionary of profiles."""
        self.profiles = [Profile(profile_name, json_root["profiles"][profile_name]) for
                         profile_name in sorted(json_root["profiles"].keys())]
        self.__set_parent_profile_refs()
        self.__mine_for_diamond_errors()

        self.virtual_profiles = [p for p in self.profiles if p.is_virtual]
        self.profiles = [p for p in self.profiles if not p.is_virtual]
        for profile in self.profiles:
            profile.merge_parent_profiles()

    def __set_parent_profile_refs(self):
        for profile1 in self.profiles:
            for profile2 in self.profiles:
                if profile1.is_parent_profile(profile2):
                    profile2.parent_profiles.add(profile1)

    def __mine_for_diamond_errors(self):
        for profile in self.profiles:
            parent_path_set = []
            if self.__has_diamonds(profile, parent_path_set):
                msg = f"Interaction Profile: {profile.name} in bindings.json has a diamond hierarchy, this is not supported."
                raise RuntimeError(msg)

    def __has_diamonds(self, profile, parent_path_set):
        if profile.name in parent_path_set:
            return True
        parent_path_set.append(profile.name)
        for parent in sorted(profile.parent_profiles, key=attrgetter("name")):
            if self.__has_diamonds(parent, parent_path_set):
                return True
        return False


header = '''// Copyright 2020-2025, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  {brief}.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Christoph Haag <christoph.haag@collabora.com>
 * @author Korcan Hussein <korcan.hussein@collabora.com>
 * @ingroup {group}
 */
'''
