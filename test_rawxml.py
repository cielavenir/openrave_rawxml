import openravepy
import openrave_rawxml

import pytest

import json
import pathlib
import os
import tempfile
import xml.etree.ElementTree as ET

openrave_rawxml.AcceptExtraField(openravepy.InterfaceType.robot,'foo')
openrave_rawxml.AcceptExtraField(openravepy.InterfaceType.robot,'foo2')

@pytest.fixture
def tmpdir():
    with tempfile.TemporaryDirectory() as tmpdir:
        yield tmpdir

def test_xml(tmpdir):
    try:
        try:
            env = openravepy.Environment()
            env.Load('../openrave/test/ikfastrobots/fail1.dae')
            robot = env.GetRobots()[0]
            env.Save(os.path.join(tmpdir, 'before.dae'))
            reader = openrave_rawxml.CreateRawXMLReadable('foo','<bar>1</bar><bla><blah>2</blah></bla>','Vendor')
            robot.SetReadableInterface('foo',reader)
            env.Save(os.path.join(tmpdir, 'after.dae'))
        finally:
            env.Destroy()
        # todo full-context check (cf readme.md)
        assert '<technique profile="Vendor">' not in pathlib.Path(os.path.join(tmpdir, 'before.dae')).read_text()
        assert '<technique profile="Vendor">' in pathlib.Path(os.path.join(tmpdir, 'after.dae')).read_text()

        try:
            env = openravepy.Environment()
            env.Load(os.path.join(tmpdir, 'after.dae'))
            robot = env.GetRobots()[0]
            params = robot.GetReadableInterface('foo')
            paramsSerialize = params.SerializeXML()
            lst = list(ET.fromstring('<ARRAY_DUMMY>'+paramsSerialize+'</ARRAY_DUMMY>'))
            xml = ''.join(ET.tostring(e, encoding='unicode') for e in lst)
            reader = openrave_rawxml.CreateRawXMLReadable('foo2',xml,'Vendor')
            robot.SetReadableInterface('foo2',reader)
            env.Save(os.path.join(tmpdir, 'after1.dae'))
        finally:
            env.Destroy()

        try:
            env = openravepy.Environment()
            env.Load(os.path.join(tmpdir, 'after1.dae'))
            robot = env.GetRobots()[0]
            assert paramsSerialize == robot.GetReadableInterface('foo').SerializeXML()
            assert paramsSerialize == robot.GetReadableInterface('foo2').SerializeXML()
        finally:
            env.Destroy()
    finally:
        openravepy.RaveDestroy()

def test_json(tmpdir):
    try:
        try:
            env = openravepy.Environment()
            env.Load('../openrave/test/ikfastrobots/fail1.dae')
            robot = env.GetRobots()[0]
            env.Save(os.path.join(tmpdir, 'before.json'))
            reader = openrave_rawxml.CreateRawJSONReadable('foo','{"bar":"1","bla":{"blah":"2"}}')
            robot.SetReadableInterface('foo',reader)
            env.Save(os.path.join(tmpdir, 'after.json'))
        finally:
            env.Destroy()
        assert '"readableInterfaces":{"foo":{"bar":"1","bla":{"blah":"2"}}}' not in pathlib.Path(os.path.join(tmpdir, 'before.json')).read_text()
        assert '"readableInterfaces":{"foo":{"bar":"1","bla":{"blah":"2"}}}' in pathlib.Path(os.path.join(tmpdir, 'after.json')).read_text()

        try:
            env = openravepy.Environment()
            env.Load(os.path.join(tmpdir, 'after.json'))
            robot = env.GetRobots()[0]
            params = robot.GetReadableInterface('foo')
            paramsSerialize = params.SerializeJSON()
            reader = openrave_rawxml.CreateRawJSONReadable('foo2',json.dumps(paramsSerialize))
            robot.SetReadableInterface('foo2',reader)
            env.Save(os.path.join(tmpdir, 'after1.json'))
        finally:
            env.Destroy()

        try:
            env = openravepy.Environment()
            env.Load(os.path.join(tmpdir, 'after1.json'))
            robot = env.GetRobots()[0]
            assert paramsSerialize == robot.GetReadableInterface('foo').SerializeJSON()
            assert paramsSerialize == robot.GetReadableInterface('foo2').SerializeJSON()
        finally:
            env.Destroy()
    finally:
        openravepy.RaveDestroy()
