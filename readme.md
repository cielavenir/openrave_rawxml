## openrave_rawxml openrave(py) plugin

- Both boost_python and pybind11 are supported.
- Both XML and JSON serializer are supported.

### Example

- creating foo field.

```
import openravepy
import openrave_rawxml

env=openravepy.Environment()
env.Load('openrave/test/ikfastrobots/fail1.dae')
robot=env.GetRobots()[0]
env.Save('before.dae')
reader=openrave_rawxml.CreateRawXMLReadable('foo','<bar>1</bar><bla><blah>2</blah></bla>','Vendor')
robot.SetReadableInterface('foo',reader)
env.Save('after.dae')
env.Destroy()
openravepy.RaveDestroy()
```

- taking diff to see what this plugin can do.

```
diff -u before.dae after.dae

@@ -1193,6 +1193,14 @@
 					</axis_info>
 				</technique_common>
 			</motion>
+			<extra type="foo">
+				<technique profile="Vendor">
+					<bar>1</bar>
+					<bla>
+						<blah>2</blah>
+					</bla>
+				</technique>
+			</extra>
 			<extra type="interface_type">
 				<technique profile="OpenRAVE">
 					<interface type="robot">GenericRobot</interface>
```

- checking reading foo field again and copying it to foo2 field.

```
import openravepy
import openrave_rawxml
import xml.etree.ElementTree as ET

openrave_rawxml.AcceptExtraField(openravepy.InterfaceType.robot,'foo')
openrave_rawxml.AcceptExtraField(openravepy.InterfaceType.robot,'foo2')

env=openravepy.Environment()
env.Load('after.dae')
robot=env.GetRobots()[0]
params=robot.GetReadableInterface('foo')
paramsSerialize=params.Serialize()
lst=ET.fromstring('<ARRAY_DUMMY>'+paramsSerialize+'</ARRAY_DUMMY>').getchildren()
xml=''.join(ET.tostring(e) for e in lst)
reader=openrave_rawxml.CreateRawXMLReadable('foo2',xml,'Vendor')
robot.SetReadableInterface('foo2',reader)
env.Save('after1.dae')
env.Destroy()

env=openravepy.Environment()
env.Load('after1.dae')
robot=env.GetRobots()[0]
assert paramsSerialize == robot.GetReadableInterface('foo').Serialize()
assert paramsSerialize == robot.GetReadableInterface('foo2').Serialize()
env.Destroy()

openravepy.RaveDestroy()
```

Of course the xml content can be customized using handy xml.etree.ElementTree module.
