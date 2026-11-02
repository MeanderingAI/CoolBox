package io.coolbox.distributed;
import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class DistributedTest {
 @Test void partitioningAndParameterServer(){try(DataPartitioner p=new DataPartitioner(5,2);ParameterServer server=new ParameterServer(2)){assertEquals(new DataPartitioner.Partition(0,3),p.getPartition(0));assertArrayEquals(new long[]{3,4},p.getIndices(1));server.setParameters("w",new double[]{1,2});server.updateParameters("w",new double[]{.5,1},.2);assertArrayEquals(new double[]{.9,1.8},server.getParameters("w"),1e-9);}}
 @Test void partitionDataAndDistributedAccuracy(){double[][] data={{1,2},{3,4},{5,6}};double[][][] parts=Distributed.partitionData(data,2);assertEquals(2,parts.length);assertEquals(2,parts[0].length);assertEquals(1,parts[1].length);try(DistributedContext context=new DistributedContext(1,0)){assertEquals(1.0,Distributed.computeDistributedAccuracy(new double[][]{{1,0},{0,1}},new double[][]{{1,0},{0,1}},context),1e-9);}}
 @Test void messageFieldsAndCommunication(){try(DistributedContext context=new DistributedContext(1,0);Message sent=new Message().type(MessageType.GRADIENT).sourceRank(0).destinationRank(0).data(new double[]{2,3}).metadata("step","4")){context.send(sent,0);try(Message received=context.receive(0)){assertEquals(MessageType.GRADIENT,received.type());assertArrayEquals(new double[]{2,3},received.data());assertEquals("4",received.metadata().get("step"));}}}
 @Test void invalidRankIsRejected(){try(DataPartitioner p=new DataPartitioner(5,2)){assertThrows(CoolBoxException.class,()->p.getPartition(-1));}}
}
